#include <DeviceManager.hpp>
#include <gtest/gtest.h>
#include <memory>

#include <D3DCommandQueue.hpp>
#include <D3DFence.hpp>

namespace Constants
{
	constexpr size_t bufferCount = 2u;
}

class CommandQueueTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void CommandQueueTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void CommandQueueTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(CommandQueueTest, CommandListTest)
{
	ID3D12Device5* device = s_deviceManager->GetDevice();

	D3DCommandList cmdList{};
	cmdList.Create(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	EXPECT_NE(cmdList.Get(), nullptr) << "CommandList creation failed.";
}

TEST_F(CommandQueueTest, BasicCommandQueueTest)
{
	ID3D12Device5* device = s_deviceManager->GetDevice();

	D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	D3DCommandQueue queue{};
	queue.Create(device, type, Constants::bufferCount);

	{
		type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

		D3DCommandQueue queue1{};
		queue1.Create(device, type, Constants::bufferCount);

		queue = std::move(queue1);
	}

	EXPECT_NE(queue.GetCommandList(1u).Get(), nullptr) << "CommandList creation failed.";

	D3DFence fence{};
	fence.Create(device);

	D3DCommandList& commandList0 = queue.GetCommandList(0u);
	commandList0.Reset();
	commandList0.Close();

	QueueSubmitBuilder<0u, 1u> submitBuilder{};
	submitBuilder.SignalFence(fence).CommandList(commandList0);

	queue.SubmitCommandLists(submitBuilder);

	fence.Wait(1u);
}

TEST_F(CommandQueueTest, CommandQueueCopyTest)
{
	ID3D12Device5* device  = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	const D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_COPY;

	D3DCommandQueue queue{};
	queue.Create(device, type, 1u);

	MemoryManager memoryManager{ adapter, device, 200_MB, 200_KB };

	Buffer testBuffer1{ device, &memoryManager, D3D12_HEAP_TYPE_UPLOAD };
	testBuffer1.Create(20_KB, D3D12_RESOURCE_STATE_COPY_SOURCE);

	Buffer testBuffer2{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
	testBuffer2.Create(20_KB, D3D12_RESOURCE_STATE_COPY_DEST);

	Texture testTexture{ device, &memoryManager, D3D12_HEAP_TYPE_DEFAULT };
	testTexture.Create2D(
		1280u, 720u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D12_RESOURCE_STATE_COPY_DEST
	);

	Buffer testBuffer3{ device, &memoryManager, D3D12_HEAP_TYPE_UPLOAD };
	testBuffer3.Create(testTexture.GetBufferSize(), D3D12_RESOURCE_STATE_COPY_SOURCE);

	D3DCommandList& cmdList = queue.GetCommandList(0u);
	{
		CommandListScope testScope{ cmdList };

		cmdList.CopyWhole(testBuffer1, testBuffer2);
		cmdList.CopyWhole(testBuffer3, testTexture, 0u);
	}

	D3DFence fence{};
	fence.Create(device);

	QueueSubmitBuilder<0u, 1u> submitBuilder{};
	submitBuilder.SignalFence(fence).CommandList(cmdList);

	queue.SubmitCommandLists(submitBuilder);

	fence.Wait(1u);
}
