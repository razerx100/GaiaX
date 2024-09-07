#include <DeviceManager.hpp>
#include <D3DHelperFunctions.hpp>
#include <gtest/gtest.h>
#include <memory>

#include <D3DCommandQueue.hpp>

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
