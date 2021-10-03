#include <GraphicsEngineDx12.hpp>
#include <DebugInfoManager.hpp>
#include <DeviceManager.hpp>

GraphicsEngineDx12::GraphicsEngineDx12(void* windowHandle) {
	InitD3DDeviceInstance();

#ifdef _DEBUG
	InitDebugInfoManagerInstance(GetD3DDeviceInstance()->GetDeviceRef());
#endif
}

GraphicsEngineDx12::~GraphicsEngineDx12() noexcept {
	CleanUpD3DDeviceInstance();
#ifdef _DEBUG
	CleanUpDebugInfoManagerInstance();
#endif
}

void GraphicsEngineDx12::SubmitCommands() {

}

void GraphicsEngineDx12::Render() {

}

void GraphicsEngineDx12::Resize(std::uint32_t width, std::uint32_t height) {

}

SRect GraphicsEngineDx12::GetMonitorCoordinates() {
	return SRect{};
}
