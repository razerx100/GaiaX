#ifndef GAIA_HPP_
#define GAIA_HPP_
#include <memory>
#include <DeviceManager.hpp>
#include <SwapChainManager.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DCommandList.hpp>
#include <D3DDebugLogger.hpp>
#include <BufferManager.hpp>
#include <ViewportAndScissorManager.hpp>
#include <DescriptorTableManager.hpp>
#include <TextureStorage.hpp>
#include <IThreadPool.hpp>
#include <ISharedDataContainer.hpp>
#include <CameraManager.hpp>
#include <DepthBuffer.hpp>
#include <D3DHeap.hpp>
#include <D3DResourceManager.hpp>
#include <UploadContainer.hpp>
#include <D3DFence.hpp>
#include <RenderEngine.hpp>
#include <VertexManager.hpp>
#include <ObjectManager.hpp>

namespace Gaia {
	// Variables
	extern std::unique_ptr<DeviceManager> device;
	extern std::unique_ptr<SwapChainManager> swapChain;
	extern std::unique_ptr<D3DCommandQueue> graphicsQueue;
	extern std::unique_ptr<D3DCommandList> graphicsCmdList;
	extern std::unique_ptr<D3DFence> graphicsFence;
	extern std::unique_ptr<D3DDebugLogger> debugLogger;
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
	extern std::unique_ptr<VertexManager> vertexManager;

	namespace Resources {
		extern std::unique_ptr<D3DHeap> uploadHeap;
		extern std::unique_ptr<D3DHeap> cpuWriteHeap;
		extern std::unique_ptr<D3DHeap> gpuOnlyHeap;
		extern std::unique_ptr<D3DHeap> cpuReadBackHeap;
		extern std::unique_ptr<DepthBuffer> depthBuffer;
		extern std::unique_ptr<D3DResourceManager> cpuWriteBuffer;
	}

	// Initialization functions
	void InitGraphicsQueueAndList(
		ObjectManager& om, ID3D12Device4* d3dDevice, std::uint32_t commandAllocatorCount
	);
	void InitCopyQueueAndList(ObjectManager& om, ID3D12Device4* d3dDevice);
	void InitComputeQueueAndList(
		ObjectManager& om, ID3D12Device4* d3dDevice, std::uint32_t commandAllocatorCount
	);
	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg);
	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg);
	void InitResources(ObjectManager& om);
	void InitRenderEngine(ObjectManager& om);
	void InitVertexManager(ObjectManager& om);
}
#endif
