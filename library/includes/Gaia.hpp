#ifndef GAIA_HPP_
#define GAIA_HPP_
#include <memory>
#include <Renderer.hpp>
#include <DeviceManager.hpp>
#include <SwapChainManager.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DCommandList.hpp>
#include <D3DDebugLogger.hpp>
#include <BufferManager.hpp>
#include <DescriptorTableManager.hpp>
#include <TextureStorage.hpp>
#include <IThreadPool.hpp>
#include <ISharedDataContainer.hpp>
#include <CameraManager.hpp>
#include <D3DHeap.hpp>
#include <D3DResourceBuffer.hpp>
#include <UploadContainer.hpp>
#include <D3DFence.hpp>
#include <RenderEngine.hpp>
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
	extern std::unique_ptr<DescriptorTableManager> descriptorTable;
	extern std::unique_ptr<TextureStorage> textureStorage;
	extern std::unique_ptr<CameraManager> cameraManager;
	extern std::unique_ptr<D3DCommandQueue> computeQueue;
	extern std::unique_ptr<D3DCommandList> computeCmdList;
	extern std::unique_ptr<D3DFence> computeFence;
	extern std::unique_ptr<RenderEngine> renderEngine;

	namespace Resources {
		extern std::unique_ptr<D3DHeap> uploadHeap;
		extern std::unique_ptr<D3DHeap> cpuWriteHeap;
		extern std::unique_ptr<D3DHeap> gpuOnlyHeap;
		extern std::unique_ptr<D3DHeap> cpuReadBackHeap;
		extern std::unique_ptr<D3DResourceBuffer> cpuWriteBuffer;
		extern std::unique_ptr<UploadContainer> uploadContainer;
	}

	// Initialization functions
	void InitGraphicsQueueAndList(
		ObjectManager& om, ID3D12Device4* d3dDevice, bool cmdList6,
		std::uint32_t commandAllocatorCount
	);
	void InitCopyQueueAndList(ObjectManager& om, ID3D12Device4* d3dDevice);
	void InitComputeQueueAndList(
		ObjectManager& om, ID3D12Device4* d3dDevice, std::uint32_t commandAllocatorCount
	);
	void InitResources(ObjectManager& om, IThreadPool& threadPool);
	void InitRenderEngine(
		ObjectManager& om, RenderEngineType engineType, ID3D12Device* d3dDevice,
		std::uint32_t frameCount
	);
}
#endif
