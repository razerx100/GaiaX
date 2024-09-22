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

	layout.AddSRVTable(0u, 10, D3D12_SHADER_VISIBILITY_AMPLIFICATION);
	EXPECT_EQ(layout.GetTotalDescriptorCount(), 10u) << "Total descriptor count isn't 10.";

	layout.AddUAVTable(2u, 12, D3D12_SHADER_VISIBILITY_AMPLIFICATION);
	EXPECT_EQ(layout.GetTotalDescriptorCount(), 22u) << "Total descriptor count isn't 22.";

	layout.AddCBVTable(1u, 11, D3D12_SHADER_VISIBILITY_AMPLIFICATION);
	EXPECT_EQ(layout.GetTotalDescriptorCount(), 33u) << "Total descriptor count isn't 33.";

	EXPECT_EQ(layout.GetDescriptorOffsetSRV(0u), 0u)
		<< "Descriptor offset for the slot 0 isn't 0.";
	EXPECT_EQ(layout.GetDescriptorOffsetUAV(2u), 10u)
		<< "Descriptor offset for the slot 1 isn't 10.";
	EXPECT_EQ(layout.GetDescriptorOffsetCBV(1u), 22u)
		<< "Descriptor offset for the slot 2 isn't 22.";
}

TEST_F(DescriptorHeapManagerTest, DescriptorLayoutTest2)
{
	D3DDescriptorLayout layout{};

	layout.AddRootCBV(1u, D3D12_SHADER_VISIBILITY_VERTEX);
	layout.AddConstants(0u, 10u, D3D12_SHADER_VISIBILITY_VERTEX);
	layout.AddRootSRV(1u, D3D12_SHADER_VISIBILITY_VERTEX);
	layout.AddRootSRV(0u, D3D12_SHADER_VISIBILITY_VERTEX);
	layout.AddCBVTable(2u, 20u, D3D12_SHADER_VISIBILITY_VERTEX);
	layout.AddSRVTable(22u, 20u, D3D12_SHADER_VISIBILITY_VERTEX);
	layout.AddSRVTable(2u, 20u, D3D12_SHADER_VISIBILITY_VERTEX);
	layout.AddConstants(22u, 10u, D3D12_SHADER_VISIBILITY_VERTEX);
	layout.AddRootUAV(0u, D3D12_SHADER_VISIBILITY_VERTEX);
	layout.AddRootUAV(1u, D3D12_SHADER_VISIBILITY_VERTEX);

	{
		EXPECT_EQ(layout.GetDescriptorOffsetCBV(0u), 0u) << "Descriptor offset exists.";
		EXPECT_EQ(layout.GetBindingIndexCBV(0u), 1u) << "Binding index isn't 1u.";
	}
	{
		EXPECT_EQ(layout.GetDescriptorOffsetCBV(1u), 0u) << "Descriptor offset exists.";
		EXPECT_EQ(layout.GetBindingIndexCBV(1u), 0u) << "Binding index isn't 0u.";
	}
	{
		EXPECT_EQ(layout.GetDescriptorOffsetCBV(2u), 0u) << "Descriptor offset isn't 0u.";
		EXPECT_EQ(layout.GetBindingIndexCBV(2u), 4u) << "Binding index isn't 4u.";
	}
	{
		EXPECT_EQ(layout.GetDescriptorOffsetCBV(22u), 0u) << "Descriptor offset exists.";
		EXPECT_EQ(layout.GetBindingIndexCBV(22u), 7u) << "Binding index isn't 7u.";
	}
	{
		EXPECT_EQ(layout.GetDescriptorOffsetSRV(0u), 0u) << "Descriptor offset exists.";
		EXPECT_EQ(layout.GetBindingIndexSRV(0u), 3u) << "Binding index isn't 3u.";
	}
	{
		EXPECT_EQ(layout.GetDescriptorOffsetSRV(1u), 0u) << "Descriptor offset exists.";
		EXPECT_EQ(layout.GetBindingIndexSRV(1u), 2u) << "Binding index isn't 2u.";
	}
	{
		EXPECT_EQ(layout.GetDescriptorOffsetSRV(2u), 40u) << "Descriptor offset isn't 40u.";
		EXPECT_EQ(layout.GetBindingIndexSRV(2u), 6u) << "Binding index isn't 6u.";
	}
	{
		EXPECT_EQ(layout.GetDescriptorOffsetSRV(22u), 20u) << "Descriptor offset isn't 20u.";
		EXPECT_EQ(layout.GetBindingIndexSRV(22u), 5u) << "Binding index isn't 5u.";
	}
	{
		EXPECT_EQ(layout.GetDescriptorOffsetUAV(0u), 0u) << "Descriptor offset exists.";
		EXPECT_EQ(layout.GetBindingIndexUAV(0u), 8u) << "Binding index isn't 8u.";
	}
	{
		EXPECT_EQ(layout.GetDescriptorOffsetUAV(1u), 0u) << "Descriptor offset exists.";
		EXPECT_EQ(layout.GetBindingIndexUAV(1u), 9u) << "Binding index isn't 9u.";
	}

	{
		std::vector<D3DDescriptorLayout> layouts{};
		layouts.emplace_back(layout);

		D3DRootSignatureDynamic rsDynamic{};

		rsDynamic.PopulateFromLayouts(layouts);
		EXPECT_NO_THROW(
			rsDynamic.CompileSignature(RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::UnboundArray)
		);
	}
}

TEST_F(DescriptorHeapManagerTest, DescriptorManagerTest)
{
	ID3D12Device* device   = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	{
		const size_t layoutCount = 3u;

		D3DDescriptorManager descriptorManger{ device, layoutCount };

		descriptorManger.AddSRVTable(0u, 1u, 5u, D3D12_SHADER_VISIBILITY_PIXEL);
		descriptorManger.AddUAVTable(0u, 0u, 1u, D3D12_SHADER_VISIBILITY_MESH);
		descriptorManger.AddUAVTable(1u, 0u, 2u, D3D12_SHADER_VISIBILITY_MESH);
		descriptorManger.AddRootSRV(2u, 0u, D3D12_SHADER_VISIBILITY_MESH);
		descriptorManger.AddRootCBV(3u, 0u, D3D12_SHADER_VISIBILITY_MESH);
		descriptorManger.AddSRVTable(0u, 2u, 1u, D3D12_SHADER_VISIBILITY_MESH);

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

		descriptorManger.SetDescriptorTableSRV(0u, 1u, 0u, true);
		descriptorManger.SetDescriptorTableUAV(0u, 0u, 0u, true);
		descriptorManger.SetDescriptorTableUAV(1u, 0u, 0u, true);
		descriptorManger.SetDescriptorTableSRV(0u, 2u, 0u, true);
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

			EXPECT_NO_THROW(
				rsDynamic.CompileSignature(RSCompileFlagBuilder{}.MeshShader(), BindlessLevel::UnboundArray)
			);

			D3DRootSignature rootSignature{};
			rootSignature.CreateSignature(device, rsDynamic);
		}
	}
}
