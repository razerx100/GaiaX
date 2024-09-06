#include <DeviceManager.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <D3DResources.hpp>
#include <D3DRootSignature.hpp>
#include <gtest/gtest.h>
#include <memory>

class DescriptorHeapManagerTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void DescriptorHeapManagerTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void DescriptorHeapManagerTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(DescriptorHeapManagerTest, DescriptorHeapTest)
{
	ID3D12Device* device = s_deviceManager->GetDevice();

	D3DDescriptorHeap rtvHeap{
		device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};
	rtvHeap.Create(10u);
	EXPECT_NE(rtvHeap.Get(), nullptr) << "RTV Heap creation failed.";

	D3DDescriptorHeap dsvHeap{
		device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};
	dsvHeap.Create(10u);
	EXPECT_NE(dsvHeap.Get(), nullptr) << "DSV Heap creation failed.";

	D3DDescriptorHeap srvHeap{
		device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};
	srvHeap.Create(10u);
	EXPECT_NE(srvHeap.Get(), nullptr) << "SRV Heap creation failed.";

	srvHeap.Create(20u);
	EXPECT_NE(srvHeap.Get(), nullptr) << "SRV Heap re-creation failed.";

	D3DDescriptorHeap srvHeap2{
		device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	};
	srvHeap2.Create(20u);
	EXPECT_NE(srvHeap2.Get(), nullptr) << "Shader visible SRV Heap creation failed.";

	srvHeap2.CopyDescriptors(srvHeap, 10u, 5u, 10u);
}

TEST_F(DescriptorHeapManagerTest, ReusableDescriptorHeapTest)
{
	ID3D12Device* device   = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	D3DReusableDescriptorHeap srvHeap{
		device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	Texture testTexture{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
	testTexture.Create2D(
		1280u, 720u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	D3D12_TEX2D_SRV tex2DSrv
	{
		.MipLevels = 1u
	};

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
	{
		.Format                  = testTexture.Format(),
		.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Texture2D               = tex2DSrv
	};

	UINT textureDescIndex  = srvHeap.CreateSRV(testTexture.Get(), srvDesc);
	UINT textureDescIndex2 = srvHeap.CreateSRV(testTexture.Get(), srvDesc);

	const D3D12_CPU_DESCRIPTOR_HANDLE textureDescHandle  = srvHeap.GetCPUHandle(textureDescIndex);
	const D3D12_CPU_DESCRIPTOR_HANDLE textureDescHandle2 = srvHeap.GetCPUHandle(textureDescIndex2);

	EXPECT_NE(textureDescHandle2.ptr, 0u) << "Texture wasn't created.";

	srvHeap.FreeDescriptor(textureDescIndex);

	UINT textureDescIndex1 = srvHeap.CreateSRV(testTexture.Get(), srvDesc);

	const D3D12_CPU_DESCRIPTOR_HANDLE textureDescHandle1 = srvHeap.GetCPUHandle(textureDescIndex1);

	EXPECT_EQ(textureDescHandle.ptr, textureDescHandle1.ptr) << "Handles are not the same.";
}

TEST_F(DescriptorHeapManagerTest, DescriptorLayoutTest)
{
	D3DDescriptorLayout layout{};

	EXPECT_EQ(layout.GetTotalDescriptorCount(), 0u) << "Total descriptor count isn't 0.";

	layout.AddSRV(0u, 10, D3D12_SHADER_VISIBILITY_AMPLIFICATION, true);
	EXPECT_EQ(layout.GetTotalDescriptorCount(), 10u) << "Total descriptor count isn't 10.";

	layout.AddUAV(2u, 12, D3D12_SHADER_VISIBILITY_AMPLIFICATION, true);
	EXPECT_EQ(layout.GetTotalDescriptorCount(), 22u) << "Total descriptor count isn't 22.";

	layout.AddCBV(1u, 11, D3D12_SHADER_VISIBILITY_AMPLIFICATION, true);
	EXPECT_EQ(layout.GetTotalDescriptorCount(), 33u) << "Total descriptor count isn't 33.";

	EXPECT_EQ(layout.GetSlotOffset(0u), 0u) << "Slot offset for the slot 0 isn't 0.";
	EXPECT_EQ(layout.GetSlotOffset(1u), 10u) << "Slot offset for the slot 1 isn't 10.";
	EXPECT_EQ(layout.GetSlotOffset(2u), 21u) << "Slot offset for the slot 2 isn't 21.";
}

TEST_F(DescriptorHeapManagerTest, DescriptorManagerTest)
{
	ID3D12Device* device   = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	{
		const size_t layoutCount = 3u;

		D3DDescriptorManager descriptorManger{ device, layoutCount };

		descriptorManger.AddSRV(0u, 1u, 5u, D3D12_SHADER_VISIBILITY_PIXEL, true);
		descriptorManger.AddUAV(0u, 0u, 1u, D3D12_SHADER_VISIBILITY_MESH, true);
		descriptorManger.AddUAV(1u, 0u, 2u, D3D12_SHADER_VISIBILITY_MESH, true);
		descriptorManger.AddSRV(2u, 0u, 1u, D3D12_SHADER_VISIBILITY_MESH, false);
		descriptorManger.AddCBV(3u, 0u, 1u, D3D12_SHADER_VISIBILITY_MESH, false);
		descriptorManger.AddSRV(0u, 2u, 1u, D3D12_SHADER_VISIBILITY_MESH, true);

		descriptorManger.CreateDescriptors();

		MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

		Buffer testBuffer1{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
		testBuffer1.Create(
			200_B, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		);

		Buffer testBuffer2{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
		testBuffer2.Create(
			200_B, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		);

		Buffer testBuffer3{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
		testBuffer3.Create(200_B, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		Buffer testBuffer4{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
		testBuffer4.Create(200_B, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		Buffer testBuffer5{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
		testBuffer5.Create(200_B, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		Texture testTexture{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
		testTexture.Create2D(
			1280u, 720u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);

		Texture testTexture1{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
		testTexture1.Create3D(
			1280u, 720u, 5u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);

		descriptorManger.SetDescriptorTable(0u, 1u, 0u, true);
		descriptorManger.SetDescriptorTable(0u, 0u, 0u, true);
		descriptorManger.SetDescriptorTable(1u, 0u, 0u, true);
		descriptorManger.SetDescriptorTable(0u, 2u, 0u, true);
		descriptorManger.SetRootSRV(2u, 0u, testBuffer3.GetGPUAddress(), true);
		descriptorManger.SetRootCBV(3u, 0u, testBuffer4.GetGPUAddress(), true);

		descriptorManger.CreateSRV(0u, 1u, 2u, testTexture.Get(), testTexture.GetSRVDesc());
		descriptorManger.CreateSRV(0u, 1u, 3u, testTexture1.Get(), testTexture1.GetSRVDesc());
		descriptorManger.CreateUAV(0u, 0u, 0u, testBuffer1.Get(), nullptr, Buffer::GetUAVDesc(10u, 20u));
		descriptorManger.CreateUAV(1u, 0u, 1u, testBuffer2.Get(), nullptr, Buffer::GetUAVDesc(10u, 20u));
		descriptorManger.CreateSRV(0u, 2u, 0u, testBuffer5.Get(), Buffer::GetSRVDesc(10u, 20u));

		{
			D3DRootSignatureDynamic rsDynamic{};

			rsDynamic.PopulateFromLayouts(descriptorManger.GetLayouts());
			rsDynamic.CompileSignature(RSCompileFlagBuilder{}.MeshShader(), BindlessLevel::UnboundArray);

			D3DRootSignature rootSignature{};
			rootSignature.CreateSignature(device, rsDynamic);
		}
	}
}
