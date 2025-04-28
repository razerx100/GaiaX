#include <gtest/gtest.h>
#include <memory>

#include <D3DRenderEngineVS.hpp>
#include <D3DRenderEngineMS.hpp>

#include <SimpleWindow.hpp>
#include <RendererDx12.hpp>

using namespace Gaia;

namespace Constants
{
	constexpr UINT width               = 1920;
	constexpr UINT height              = 1080u;
	constexpr std::uint32_t frameCount = 2u;
	constexpr const char* appName      = "GaiaTest";
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

TEST(RendererVKTest, RendererTest)
{
	SimpleWindow window{ Constants::width, Constants::height, Constants::appName };

	RendererDx12<RenderEngineMS> renderer{
		window.GetWindowHandle(), Constants::width, Constants::height, Constants::frameCount,
		std::make_shared<ThreadPool>(8u)
	};
}
