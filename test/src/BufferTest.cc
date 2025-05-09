#include <D3DDeviceManager.hpp>
#include <gtest/gtest.h>
#include <memory>

#include <D3DResources.hpp>
#include <D3DCameraManager.hpp>
#include <D3DMultiInstanceBuffer.hpp>

using namespace Gaia;

namespace Constants
{
	constexpr size_t bufferCount = 2u;
}

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

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
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

	EXPECT_EQ(testTexel.BufferSize(), 2_KB) << "Buffer size isn't 2_KB.";
	EXPECT_NE(testTexel.Get(), nullptr) << "Buffer wasn't created.";

	testTexel.Create(4_KB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	EXPECT_EQ(testTexel.BufferSize(), 4_KB) << "Buffer size isn't 2_KB.";
	EXPECT_NE(testTexel.Get(), nullptr) << "Buffer wasn't created.";
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

	EXPECT_EQ(testTexture.GetWidth(), 1280u) << "Texture width isn't 1280.";
	EXPECT_EQ(testTexture.GetHeight(), 720u) << "Texture width isn't 720.";
	EXPECT_EQ(testTexture.GetBufferSize(), 3'686'400lu) << "Texture size isn't 3'686'400.";
	EXPECT_NE(testTexture.Get(), nullptr) << "Texture wasn't created.";

	testTexture.Create2D(
		1920u, 1080u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	EXPECT_EQ(testTexture.GetWidth(), 1920u) << "Texture width isn't 1920.";
	EXPECT_EQ(testTexture.GetHeight(), 1080u) << "Texture width isn't 1080.";
	EXPECT_EQ(testTexture.GetBufferSize(), 8'294'400lu) << "Texture size isn't 8'294'400.";
	EXPECT_NE(testTexture.Get(), nullptr) << "Texture wasn't created.";
}
TEST_F(BufferTest, CameraManagerTest)
{
	ID3D12Device* device   = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	CameraManager cameraManager{ device, &memoryManager };

	cameraManager.CreateBuffer(Constants::bufferCount);

	std::vector<D3DDescriptorManager> descriptorManagers{};
	descriptorManagers.emplace_back(device, Constants::bufferCount);

	cameraManager.SetDescriptorLayoutGraphics(descriptorManagers, 0u, 0u, D3D12_SHADER_VISIBILITY_VERTEX);

	for (auto& descriptorManager : descriptorManagers)
		descriptorManager.CreateDescriptors();

	cameraManager.SetDescriptorGraphics(descriptorManagers, 0u, 0u);
}

TEST_F(BufferTest, ReusableBufferTest)
{
	ID3D12Device* device   = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	MultiInstanceCPUBuffer reusableCpuMBuffer{
		device, &memoryManager, Constants::bufferCount, static_cast<std::uint32_t>(sizeof(size_t))
	};

	reusableCpuMBuffer.AllocateForIndex(1u);
}
