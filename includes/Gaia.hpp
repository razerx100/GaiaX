#ifndef GAIA_HPP_
#define GAIA_HPP_
#include <memory>
#include <DeviceManager.hpp>
#include <SwapChainManager.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DCommandList.hpp>
#include <D3DDebugLogger.hpp>
#include <RenderPipeline.hpp>
#include <BufferManager.hpp>
#include <ViewportAndScissorManager.hpp>
#include <DescriptorTableManager.hpp>
#include <TextureStorage.hpp>
#include <IThreadPool.hpp>
#include <ISharedDataContainer.hpp>
#include <CameraManager.hpp>
#include <DepthBuffer.hpp>
#include <D3DHeap.hpp>
#include <D3DSingleResourceManager.hpp>
#include <D3DUploadableResourceManager.hpp>
#include <UploadContainer.hpp>
#include <D3DFence.hpp>
#include <RenderEngine.hpp>

namespace Gaia {
	// Variables
	extern std::unique_ptr<DeviceManager> device;
	extern std::unique_ptr<SwapChainManager> swapChain;
	extern std::unique_ptr<D3DCommandQueue> graphicsQueue;
	extern std::unique_ptr<D3DCommandList> graphicsCmdList;
	extern std::unique_ptr<D3DFence> graphicsFence;
	extern std::unique_ptr<D3DDebugLogger> debugLogger;
	extern std::unique_ptr<RenderPipeline> renderPipeline;
	extern std::unique_ptr<BufferManager> bufferManager;
	extern std::unique_ptr<D3DCommandQueue> copyQueue;
	extern std::unique_ptr<D3DCommandList> copyCmdList;
	extern std::unique_ptr<ViewportAndScissorManager> viewportAndScissor;
	extern std::unique_ptr<DescriptorTableManager> descriptorTable;
	extern std::unique_ptr<TextureStorage> textureStorage;
	extern std::shared_ptr<IThreadPool> threadPool;
	extern std::unique_ptr<CameraManager> cameraManager;
	extern std::shared_ptr<ISharedDataContainer> sharedData;
	extern std::unique_ptr<D3DCommandQueue> computeQueue;
	extern std::unique_ptr<D3DCommandList> computeCmdList;
	extern std::unique_ptr<D3DFence> computeFence;
	extern std::unique_ptr<RenderEngine> renderEngine;

	namespace Resources {
		extern std::unique_ptr<D3DHeap> uploadHeap;
		extern std::unique_ptr<D3DHeap> cpuWriteHeap;
		extern std::unique_ptr<D3DHeap> gpuOnlyHeap;
		extern std::unique_ptr<D3DHeap> cpuReadBackHeap;
		extern std::unique_ptr<DepthBuffer> depthBuffer;
		extern std::unique_ptr<D3DUploadableResourceManager> vertexBuffer;
		extern std::unique_ptr<UploadContainer> vertexUploadContainer;
		extern std::unique_ptr<UploadContainerTexture> textureUploadContainer;
		extern std::unique_ptr<D3DSingleResourceManager> cpuWriteBuffer;
	}

	// Initialization functions
	void InitDevice();
	void InitSwapChain(const SwapChainCreateInfo& createInfo);
	void InitGraphicsQueueAndList(ID3D12Device4* d3dDevice, std::uint32_t commandAllocatorCount);
	void InitDebugLogger(ID3D12Device* d3dDevice);
	void InitDepthBuffer(ID3D12Device* d3dDevice);
	void InitCopyQueueAndList(ID3D12Device4* d3dDevice);
	void InitComputeQueueAndList(ID3D12Device4* d3dDevice, std::uint32_t commandAllocatorCount);
	void InitViewportAndScissor(std::uint32_t width, std::uint32_t height);
	void InitDescriptorTable();
	void InitTextureStorage();
	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg);
	void InitCameraManager();
	void InitRenderPipeline(std::uint32_t bufferCount);
	void InitBufferManager(std::uint32_t bufferCount);
	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg);
	void InitResources();
	void InitRenderEngine();
	void CleanUpResources();
	void CleanUpUploadResources();
}
#endif
