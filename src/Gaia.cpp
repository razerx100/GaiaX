#include <Gaia.hpp>

namespace Gaia {
	std::unique_ptr<DeviceManager> device;
	std::unique_ptr<SwapChainManager> swapChain;
	std::unique_ptr<GraphicsQueueManager> graphicsQueue;
	std::unique_ptr<CommandListManager> graphicsCmdList;
	std::unique_ptr<DebugInfoManager> debugInfo;
	std::unique_ptr<DepthBuffer> depthBuffer;
	std::unique_ptr<ModelContainer> modelContainer;
	std::unique_ptr<CopyQueueManager> copyQueue;
	std::unique_ptr<CommandListManager> copyCmdList;
	std::unique_ptr<ResourceBuffer> vertexBuffer;
	std::unique_ptr<ResourceBuffer> indexBuffer;
	std::unique_ptr<ViewportAndScissorManager> viewportAndScissor;
	std::unique_ptr<HeapManager> heapManager;
	std::unique_ptr<DescriptorTableManager> descriptorTable;
	std::unique_ptr<TextureStorage> textureStorage;
	std::shared_ptr<IThreadPool> threadPool;
	std::unique_ptr<CPUAccessibleStorage> constantBuffer;
	std::unique_ptr<CameraManager> cameraManager;
	std::shared_ptr<ISharedDataContainer> sharedData;

	void InitDevice() {
		device = std::make_unique<DeviceManager>();
	}

	void InitSwapChain(const SwapChainCreateInfo& createInfo) {
		swapChain = std::make_unique<SwapChainManager>(createInfo);
	}

	void InitGraphicsQueue(ID3D12Device* d3dDevice, std::uint32_t syncObjCount) {
		graphicsQueue = std::make_unique<GraphicsQueueManager>(d3dDevice, syncObjCount);
	}

	void InitGraphicsCmdList(ID3D12Device4* d3dDevice, std::uint32_t listCount) {
		graphicsCmdList = std::make_unique<CommandListManager>(
			d3dDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, listCount
			);
	}

	void InitDebugInfo() {
		debugInfo = std::make_unique<DebugInfoManager>();
	}

	void InitDepthBuffer(ID3D12Device* d3dDevice) {
		depthBuffer = std::make_unique<DepthBuffer>(d3dDevice);
	}

	void InitModelContainer(const std::string& shaderPath) {
		modelContainer = std::make_unique<ModelContainer>(shaderPath);
	}

	void InitCopyQueue(ID3D12Device* d3dDevice) {
		copyQueue = std::make_unique<CopyQueueManager>(d3dDevice);
	}

	void InitCopyCmdList(ID3D12Device4* d3dDevice) {
		copyCmdList = std::make_unique<CommandListManager>(
			d3dDevice, D3D12_COMMAND_LIST_TYPE_COPY, 1u
			);
	}

	void InitVertexBuffer() {
		vertexBuffer = std::make_unique<ResourceBuffer>();
	}

	void InitIndexBuffer() {
		indexBuffer = std::make_unique<ResourceBuffer>();
	}

	void InitViewportAndScissor(std::uint32_t width, std::uint32_t height) {
		viewportAndScissor = std::make_unique<ViewportAndScissorManager>(width, height);
	}

	void InitHeapManager() {
		heapManager = std::make_unique<HeapManager>();
	}

	void InitDescriptorTable() {
		descriptorTable = std::make_unique<DescriptorTableManager>();
	}

	void InitTextureStorage() {
		textureStorage = std::make_unique<TextureStorage>();
	}

	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg) {
		threadPool = std::move(threadPoolArg);
	}

	void InitConstantBuffer() {
		constantBuffer = std::make_unique<CPUAccessibleStorage>();
	}

	void InitCameraManager() {
		cameraManager = std::make_unique<CameraManager>();
	}

	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg) {
		sharedData = std::move(sharedDataArg);
	}
}
