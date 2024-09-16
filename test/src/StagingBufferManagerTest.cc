#include <DeviceManager.hpp>
#include <gtest/gtest.h>
#include <memory>

#include <D3DResources.hpp>
#include <StagingBufferManager.hpp>

namespace Constants
{
	constexpr size_t frameCount = 2u;
}

class StagingBufferTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void StagingBufferTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void StagingBufferTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(StagingBufferTest, StagingTest)
{
	ID3D12Device5* device  = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	D3DCommandQueue copyQueue{};
	copyQueue.Create(device, D3D12_COMMAND_LIST_TYPE_COPY, Constants::frameCount);

	ThreadPool threadPool{ 8u };

	StagingBufferManager stagingBufferMan{ device, &memoryManager, &threadPool };

	Buffer testNonPixel{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
	testNonPixel.Create(2_KB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	Texture testTexture{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
	testTexture.Create2D(
		1280u, 720u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D12_RESOURCE_STATE_COMMON
	);

	auto bufferData                = std::make_unique<std::uint8_t[]>(2_KB);
	const UINT64 textureBufferSize = testTexture.GetBufferSize();

	auto textureData               = std::make_unique<std::uint8_t[]>(textureBufferSize);

	TemporaryDataBufferGPU tempDataBuffer{};

	stagingBufferMan.AddBuffer(std::move(bufferData), 2_KB, &testNonPixel, 0u, tempDataBuffer);
	stagingBufferMan.AddTexture(std::move(textureData), &testTexture, tempDataBuffer);

	const D3DCommandList& copyCmdList = copyQueue.GetCommandList(0u);

	{
		const CommandListScope cmdListScope{ copyCmdList };

		stagingBufferMan.CopyAndClear(cmdListScope);
	}

	D3DFence waitFence{};
	waitFence.Create(device);

	QueueSubmitBuilder<0u, 1u> submitBuilder{};
	submitBuilder.SignalFence(waitFence).CommandList(copyCmdList);

	copyQueue.SubmitCommandLists(submitBuilder);

	waitFence.Wait(1u);
}
