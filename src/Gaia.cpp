#include <Gaia.hpp>

namespace Gaia {
	std::unique_ptr<DeviceManager> device;
	std::unique_ptr<SwapChainManager> swapChain;
	std::unique_ptr<GraphicsQueueManager> graphicsQueue;
	std::unique_ptr<CommandListManager> graphicsCmdList;
	std::unique_ptr<DebugInfoManager> debugInfo;
	std::unique_ptr<ModelContainer> modelContainer;
	std::unique_ptr<CopyQueueManager> copyQueue;
	std::unique_ptr<CommandListManager> copyCmdList;
	std::unique_ptr<ViewportAndScissorManager> viewportAndScissor;
	std::unique_ptr<HeapManager> heapManager;
	std::unique_ptr<DescriptorTableManager> descriptorTable;
	std::unique_ptr<TextureStorage> textureStorage;
	std::shared_ptr<IThreadPool> threadPool;
	std::unique_ptr<CameraManager> cameraManager;
	std::shared_ptr<ISharedDataContainer> sharedData;

	namespace Resources {
		std::unique_ptr<D3DHeap> uploadHeap;
		std::unique_ptr<D3DHeap> cpuWriteHeap;
		std::unique_ptr<D3DHeap> gpuOnlyHeap;
		std::unique_ptr<D3DHeap> cpuReadBackHeap;
		std::unique_ptr<DepthBuffer> depthBuffer;
		std::unique_ptr<D3DUploadableResourceManager> vertexBuffer;
		std::unique_ptr<D3DSingleResourceManager> cpuWriteBuffer;
	}

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
		Resources::depthBuffer = std::make_unique<DepthBuffer>(d3dDevice);
	}

	void InitModelContainer(const std::string& shaderPath, std::uint32_t bufferCount) {
		modelContainer = std::make_unique<ModelContainer>(shaderPath, bufferCount);
	}

	void InitCopyQueue(ID3D12Device* d3dDevice) {
		copyQueue = std::make_unique<CopyQueueManager>(d3dDevice);
	}

	void InitCopyCmdList(ID3D12Device4* d3dDevice) {
		copyCmdList = std::make_unique<CommandListManager>(
			d3dDevice, D3D12_COMMAND_LIST_TYPE_COPY, 1u
			);
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

	void InitCameraManager() {
		cameraManager = std::make_unique<CameraManager>();
	}

	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg) {
		sharedData = std::move(sharedDataArg);
	}

	void InitResources() {
		Resources::uploadHeap = std::make_unique<D3DHeap>(D3D12_HEAP_TYPE_UPLOAD);
		Resources::cpuWriteHeap = std::make_unique<D3DHeap>(D3D12_HEAP_TYPE_UPLOAD);
		Resources::gpuOnlyHeap = std::make_unique<D3DHeap>(D3D12_HEAP_TYPE_DEFAULT);
		Resources::cpuReadBackHeap = std::make_unique<D3DHeap>(D3D12_HEAP_TYPE_READBACK);
		Resources::vertexBuffer = std::make_unique<D3DUploadableResourceManager>();
		Resources::cpuWriteBuffer = std::make_unique<D3DSingleResourceManager>(
			ResourceType::cpuWrite
			);
	}

	void CleanUpResources() {
		Resources::depthBuffer.reset();
		Resources::cpuWriteBuffer.reset();
		Resources::vertexBuffer.reset();
		Resources::cpuWriteHeap.reset();
		Resources::gpuOnlyHeap.reset();
		Resources::cpuReadBackHeap.reset();
	}
}
