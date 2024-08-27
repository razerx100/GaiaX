#include <DeviceManager.hpp>
#include <D3DHelperFunctions.hpp>
#include <gtest/gtest.h>
#include <memory>

#include <D3DResources.hpp>

class BufferTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void BufferTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void BufferTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(BufferTest, D3DBufferTest)
{
	ID3D12Device* device   = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	Buffer testStorage{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
	testStorage.Create(2_KB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	Buffer testIndirect{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
	testIndirect.Create(2_KB, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	Buffer testTexel{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
	testTexel.Create(2_KB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	testTexel.Create(4_KB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

TEST_F(BufferTest, TextureTest)
{
	ID3D12Device* device   = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	Texture testTexture{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
	testTexture.Create2D(
		1280u, 720u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	testTexture.Create2D(
		1920u, 1080u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
}
