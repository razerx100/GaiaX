#include <DeviceManager.hpp>
#include <GraphicsEngineDx12.hpp>
#include <SwapChainManager.hpp>
#include <GraphicsQueueManager.hpp>
#include <CommandListManager.hpp>

static DeviceManager* s_pD3DDevice = nullptr;

static GraphicsEngine* s_pGraphicsEngine = nullptr;

static ISwapChainManager* s_pSwapChainManager = nullptr;

static IGraphicsQueueManager* s_pGraphicsQueue = nullptr;

static ICommandListManager* s_pGraphicsList = nullptr;

IDeviceManager* GetD3DDeviceInstance() noexcept {
	return s_pD3DDevice;
}

void InitD3DDeviceInstance() {
	if(!s_pD3DDevice)
		s_pD3DDevice = new DeviceManager();
}

void CleanUpD3DDeviceInstance() noexcept {
	if (s_pD3DDevice) {
		delete s_pD3DDevice;
		s_pD3DDevice = nullptr;
	}
}

GraphicsEngine* GetGraphicsEngineInstance() noexcept {
	return s_pGraphicsEngine;
}

void InitGraphicsEngineInstance(
	const char* appName,
	void* windowHandle,
	void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) {
	if(!s_pGraphicsEngine)
		s_pGraphicsEngine = new GraphicsEngineDx12(
			appName,
			windowHandle, width, height, bufferCount
		);

	// Useless here. Necessary for Vulkan
	moduleHandle = nullptr;
}

void CleanUpGraphicsEngineInstance() noexcept {
	if (s_pGraphicsEngine) {
		delete s_pGraphicsEngine;
		s_pGraphicsEngine = nullptr;
	}
}

ISwapChainManager* GetSwapChainInstance() noexcept {
	return s_pSwapChainManager;
}

void InitSwapChianInstance(
	IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
	std::uint8_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	bool variableRefreshRateAvailable
) {
	if (!s_pSwapChainManager)
		s_pSwapChainManager = new SwapChainManager(
			factory, cmdQueue, windowHandle,
			bufferCount, width, height,
			variableRefreshRateAvailable
		);
}

void CleanUpSwapChainInstance() noexcept {
	if (s_pSwapChainManager) {
		delete s_pSwapChainManager;
		s_pSwapChainManager = nullptr;
	}
}

IGraphicsQueueManager* GetGraphicsQueueInstance() noexcept {
	return s_pGraphicsQueue;
}

void InitGraphicsQueueInstance(
	ID3D12Device5* device,
	std::uint8_t bufferCount
) {
	if (!s_pGraphicsQueue)
		s_pGraphicsQueue = new GraphicsQueueManager(
			device,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			bufferCount
		);
}

void CleanUpGraphicsQueueInstance() noexcept {
	if (s_pGraphicsQueue) {
		delete s_pGraphicsQueue;
		s_pGraphicsQueue = nullptr;
	}
}

ICommandListManager* GetGraphicsListInstance() noexcept {
	return s_pGraphicsList;
}

void InitGraphicsListInstance(
	ID3D12Device5* device,
	std::uint8_t bufferCount
) {
	if (!s_pGraphicsList)
		s_pGraphicsList = new CommandListManager(
			device,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			bufferCount
		);
}

void CleanUpGraphicsListInstance() noexcept {
	if (s_pGraphicsList) {
		delete s_pGraphicsList;
		s_pGraphicsList = nullptr;
	}
}
