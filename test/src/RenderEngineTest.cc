#include <gtest/gtest.h>
#include <memory>

#include <D3DRenderEngineVS.hpp>
#include <D3DRenderEngineMS.hpp>

using namespace Gaia;

namespace Constants
{
	constexpr std::uint32_t frameCount = 2u;
}

class RenderEngineTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void RenderEngineTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void RenderEngineTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(RenderEngineTest, RenderEngineVSIndividualTest)
{
	auto threadPool = std::make_shared<ThreadPool>(2u);

	RenderEngineVSIndividual renderEngine{ *s_deviceManager, threadPool, Constants::frameCount };
}

TEST_F(RenderEngineTest, RenderEngineVSIndirectTest)
{
	auto threadPool = std::make_shared<ThreadPool>(2u);

	RenderEngineVSIndirect renderEngine{ *s_deviceManager, threadPool, Constants::frameCount };
}

TEST_F(RenderEngineTest, RenderEngineMSTest)
{
	auto threadPool = std::make_shared<ThreadPool>(2u);

	RenderEngineMS renderEngine{ *s_deviceManager, threadPool, Constants::frameCount };
}
