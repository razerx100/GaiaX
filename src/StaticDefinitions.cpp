#include <DeviceManager.hpp>
#include <GraphicsEngineDx12.hpp>

static DeviceManager* s_pD3DDevice = nullptr;

static GraphicsEngine* s_pGraphicsEngine = nullptr;

DeviceManager* GetD3DDeviceInstance() noexcept {
	return s_pD3DDevice;
}

void InitD3DDeviceInstance() {
	if(!s_pD3DDevice)
		s_pD3DDevice = new DeviceManager();
}

void CleanUpD3DDeviceInstance() {
	if (s_pD3DDevice)
		delete s_pD3DDevice;
}

GraphicsEngine* GetGraphicsEngineInstance() noexcept {
	return s_pGraphicsEngine;
}

void InitGraphicsEngineInstance(void* windowHandle) {
	if(!s_pGraphicsEngine)
		s_pGraphicsEngine = new GraphicsEngineDx12(windowHandle);
}

void CleanUpGraphicsEngineInstance() {
	if (s_pGraphicsEngine)
		delete s_pGraphicsEngine;
}
