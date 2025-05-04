#include <gtest/gtest.h>
#include <memory>
#include <limits>
#include <ranges>
#include <algorithm>

#include <D3DDeviceManager.hpp>

#include <D3DExternalRenderPass.hpp>
#include <D3DExternalBuffer.hpp>

using namespace Gaia;

namespace Constants
{
	constexpr const char* appName      = "GaiaTest";
	constexpr std::uint32_t frameCount = 2u;
}

class ExternalResourceTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void ExternalResourceTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void ExternalResourceTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(ExternalResourceTest, D3DExternalRenderPassTest)
{
	ID3D12Device* device = s_deviceManager->GetDevice();

	D3DReusableDescriptorHeap rtvHeap
	{
		device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	D3DReusableDescriptorHeap dsvHeap
	{
		device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	auto vkRenderPass = std::make_shared<D3DExternalRenderPass>(&rtvHeap, &dsvHeap);

	ExternalRenderPass<D3DExternalRenderPass> renderPass{ vkRenderPass };
}

TEST_F(ExternalResourceTest, D3DExternalBufferTest)
{
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();
	ID3D12Device* device   = s_deviceManager->GetDevice();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	auto d3dBuffer = std::make_shared<D3DExternalBuffer>(
		device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);

	ExternalBuffer<D3DExternalBuffer> buffer{ d3dBuffer };
}

TEST_F(ExternalResourceTest, D3DExternalTextureTest)
{
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();
	ID3D12Device* device   = s_deviceManager->GetDevice();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	auto d3dTexture = std::make_shared<D3DExternalTexture>(device, &memoryManager);

	ExternalTexture<D3DExternalTexture> texture{ d3dTexture };
}
