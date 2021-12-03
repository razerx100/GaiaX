#include <InstanceManager.hpp>

void DeviceInst::Init() {
	Set(
		CreateD3DDeviceInstance()
	);
}

void SwapchainInst::Init(
	IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
	std::uint8_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	bool variableRefreshRateAvailable
) {
	Set(
		CreateSwapChainInstance(
			factory, cmdQueue, windowHandle, bufferCount,
			width, height, variableRefreshRateAvailable
		)
	);
}

void GfxQueInst::Init(
	ID3D12Device5* device,
	std::uint8_t bufferCount
) {
	Set(
		CreateGraphicsQueueInstance(
			device, bufferCount
		)
	);
}

void GfxCmdListInst::Init(
	ID3D12Device5* device,
	std::uint8_t bufferCount
) {
	Set(
		CreateCommandListInstance(
			device,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			bufferCount
		)
	);
}

void DebugInfoInst::Init() {
	Set(
		CreateDebugInfoManagerInstance()
	);
}
