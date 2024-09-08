#include <DeviceManager.hpp>
#include <D3DHelperFunctions.hpp>
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
