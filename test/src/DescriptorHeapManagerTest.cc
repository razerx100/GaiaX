#include <DeviceManager.hpp>
#include <D3DDescriptorHeapManager.hpp>
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

	srvHeap2.CopyHeap(srvHeap, 10u, 5u, 10u);
}
