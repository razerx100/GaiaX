#include <DeviceManager.hpp>
#include <gtest/gtest.h>
#include <memory>

#include <SwapchainManager.hpp>
#include <SimpleWindow.hpp>

namespace Constants
{
	constexpr size_t bufferCount  = 2u;
	constexpr UINT width          = 1920;
	constexpr UINT height         = 1080u;
	constexpr const char* appName = "GaiaTest";
}

class SwapchainTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
	inline static std::unique_ptr<SimpleWindow>  s_window;
};

void SwapchainTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();

	s_window = std::make_unique<SimpleWindow>(
		Constants::width, Constants::height, Constants::appName
	);

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void SwapchainTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

TEST_F(SwapchainTest, SwapchainManagerTest)
{
	ID3D12Device5* device  = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 100_MB, 20_MB };

	D3DCommandQueue presentQueue{};
	presentQueue.Create(device, D3D12_COMMAND_LIST_TYPE_DIRECT, Constants::bufferCount);

	D3DReusableDescriptorHeap rtvHeap{
		device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	SwapchainManager swapchain{
		&rtvHeap, static_cast<UINT>(Constants::bufferCount),
		s_deviceManager->GetFactory(), presentQueue,
		static_cast<HWND>(s_window->GetWindowHandle())
	};

	swapchain.Resize(Constants::width, Constants::height);

	for (size_t index = 0u; index < Constants::bufferCount; ++index)
		EXPECT_NE(swapchain.GetRenderTarget(index), nullptr) << "Render target creation failed.";

	s_window->SetWindowResolution(2560u, 1440u);

	swapchain.Resize(2560u, 1440u);

	for (size_t index = 0u; index < Constants::bufferCount; ++index)
		EXPECT_NE(swapchain.GetRenderTarget(index), nullptr) << "Render target re-creation failed.";
}
