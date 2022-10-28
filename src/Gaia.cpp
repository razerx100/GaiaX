#include <Gaia.hpp>

namespace Gaia {
	std::unique_ptr<DeviceManager> device;
	std::unique_ptr<SwapChainManager> swapChain;
	std::unique_ptr<D3DCommandQueue> graphicsQueue;
	std::unique_ptr<D3DCommandList> graphicsCmdList;
	std::unique_ptr<D3DFence> graphicsFence;
	std::unique_ptr<DebugInfoManager> debugInfo;
	std::unique_ptr<D3DDebugLogger> debugLogger;
	std::unique_ptr<RenderPipeline> renderPipeline;
	std::unique_ptr<BufferManager> bufferManager;
	std::unique_ptr<D3DCommandQueue> copyQueue;
	std::unique_ptr<D3DCommandList> copyCmdList;
	std::unique_ptr<ViewportAndScissorManager> viewportAndScissor;
	std::unique_ptr<DescriptorTableManager> descriptorTable;
	std::unique_ptr<TextureStorage> textureStorage;
	std::shared_ptr<IThreadPool> threadPool;
	std::unique_ptr<CameraManager> cameraManager;
	std::shared_ptr<ISharedDataContainer> sharedData;
	std::unique_ptr<D3DCommandQueue> computeQueue;
	std::unique_ptr<D3DCommandList> computeCmdList;
	std::unique_ptr<D3DFence> computeFence;

	namespace Resources {
		std::unique_ptr<D3DHeap> uploadHeap;
		std::unique_ptr<D3DHeap> cpuWriteHeap;
		std::unique_ptr<D3DHeap> gpuOnlyHeap;
		std::unique_ptr<D3DHeap> cpuReadBackHeap;
		std::unique_ptr<DepthBuffer> depthBuffer;
		std::unique_ptr<D3DUploadableResourceManager> vertexBuffer;
		std::unique_ptr<UploadContainer> vertexUploadContainer;
		std::unique_ptr<UploadContainerTexture> textureUploadContainer;
		std::unique_ptr<D3DSingleResourceManager> cpuWriteBuffer;
	}

	void InitDevice() {
		device = std::make_unique<DeviceManager>();
	}

	void InitSwapChain(const SwapChainCreateInfo& createInfo) {
		swapChain = std::make_unique<SwapChainManager>(createInfo);
	}

	void InitGraphicsQueueAndList(
		ID3D12Device4* d3dDevice, std::uint32_t commandAllocatorCount
	) {
		graphicsQueue = std::make_unique<D3DCommandQueue>(
			d3dDevice, D3D12_COMMAND_LIST_TYPE_DIRECT
			);
		graphicsCmdList = std::make_unique<D3DCommandList>(
			d3dDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocatorCount
			);
		graphicsFence = std::make_unique<D3DFence>(d3dDevice, commandAllocatorCount);
	}

	void InitDebugInfo() {
		debugInfo = std::make_unique<DebugInfoManager>();
	}

	void InitDebugLogger(ID3D12Device* d3dDevice) {
		debugLogger = std::make_unique<D3DDebugLogger>(d3dDevice);
	}

	void InitDepthBuffer(ID3D12Device* d3dDevice) {
		Resources::depthBuffer = std::make_unique<DepthBuffer>(d3dDevice);
	}

	void InitRenderPipeline(std::uint32_t bufferCount) {
		renderPipeline = std::make_unique<RenderPipeline>(bufferCount);
	}

	void InitBufferManager(std::uint32_t bufferCount) {
		bufferManager = std::make_unique<BufferManager>(bufferCount);
	}

	void InitCopyQueueAndList(ID3D12Device4* d3dDevice) {
		copyQueue = std::make_unique<D3DCommandQueue>(d3dDevice, D3D12_COMMAND_LIST_TYPE_COPY);
		copyCmdList = std::make_unique<D3DCommandList>(d3dDevice, D3D12_COMMAND_LIST_TYPE_COPY);
	}

	void InitComputeQueueAndList(
		ID3D12Device4* d3dDevice, std::uint32_t commandAllocatorCount
	) {
		computeQueue = std::make_unique<D3DCommandQueue>(
			d3dDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE
			);
		computeCmdList = std::make_unique<D3DCommandList>(
			d3dDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE, commandAllocatorCount
			);
		computeFence = std::make_unique<D3DFence>(d3dDevice, commandAllocatorCount);
	}

	void InitViewportAndScissor(std::uint32_t width, std::uint32_t height) {
		viewportAndScissor = std::make_unique<ViewportAndScissorManager>(width, height);
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
		Resources::vertexUploadContainer = std::make_unique<UploadContainer>();
		Resources::textureUploadContainer = std::make_unique<UploadContainerTexture>();
		Resources::cpuWriteBuffer = std::make_unique<D3DSingleResourceManager>(
			ResourceType::cpuWrite
			);
	}

	void CleanUpUploadResources() {
		Resources::vertexUploadContainer.reset();
		Resources::textureUploadContainer.reset();
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
