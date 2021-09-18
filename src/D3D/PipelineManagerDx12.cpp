#include <PipelineManagerDx12.hpp>
#include <DebugInfoManager.hpp>
#include <DeviceManager.hpp>

PipelineManagerDx12::PipelineManagerDx12(void* windowHandle) {
	DeviceManager::Init();

#ifdef _DEBUG
	DebugInfoManager::SetDebugInfoManager(DeviceManager::GetRef()->GetDeviceRef());
#endif
}

void PipelineManagerDx12::SubmitCommands() {

}

void PipelineManagerDx12::Render() {

}

void PipelineManagerDx12::Resize(std::uint32_t width, std::uint32_t height) {

}

SRect PipelineManagerDx12::GetMonitorCoordinates() {
	return SRect{};
}
