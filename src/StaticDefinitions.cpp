#include <DeviceManager.hpp>
#include <SwapChainManager.hpp>
#include <GraphicsQueueManager.hpp>
#include <CommandListManager.hpp>
#include <DepthBuffer.hpp>
#include <ModelContainer.hpp>
#include <CopyQueueManager.hpp>
#include <ResourceBuffer.hpp>

IDeviceManager* CreateD3DDeviceInstance() {
	return new DeviceManager();
}

ISwapChainManager* CreateSwapChainInstance(
	IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
	std::uint8_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	bool variableRefreshRateAvailable
) {
	return new SwapChainManager(
		factory, cmdQueue, windowHandle,
		bufferCount, width, height,
		variableRefreshRateAvailable
	);
}

IGraphicsQueueManager* CreateGraphicsQueueInstance(
	ID3D12Device* device,
	std::uint8_t bufferCount
) {
	return new GraphicsQueueManager(
		device,
		bufferCount
	);
}

ICommandListManager* CreateCommandListInstance(
	ID3D12Device5* device,
	D3D12_COMMAND_LIST_TYPE type,
	std::uint8_t bufferCount
) {
	return new CommandListManager(
		device,
		type,
		bufferCount
	);
}

IDepthBuffer* CreateDepthBufferInstance(ID3D12Device* device) {
	return new DepthBuffer(device);
}

IModelContainer* CreateModelContainerInstance(
	const char* shaderPath
) {
	return new ModelContainer(shaderPath);
}

ICopyQueueManager* CreateCopyQueueInstance(
	ID3D12Device* device
) {
	return new CopyQueueManager(device);
}

IResourceBuffer* CreateResourceBufferInstance() {
	return new ResourceBuffer();
}
