#ifndef GAIA_HPP_
#define GAIA_HPP_
#include <memory>
#include <DeviceManager.hpp>
#include <SwapChainManager.hpp>
#include <GraphicsQueueManager.hpp>
#include <CommandListManager.hpp>
#include <DebugInfoManager.hpp>
#include <DepthBuffer.hpp>
#include <ModelContainer.hpp>
#include <CopyQueueManager.hpp>
#include <ResourceBuffer.hpp>
#include <ViewportAndScissorManager.hpp>
#include <HeapManager.hpp>
#include <DescriptorTableManager.hpp>
#include <TextureStorage.hpp>
#include <IThreadPool.hpp>
#include <CPUAccessibleStorage.hpp>
#include <ISharedDataContainer.hpp>
#include <CameraManager.hpp>

namespace Gaia {
	// Variables
	extern std::unique_ptr<DeviceManager> device;
	extern std::unique_ptr<SwapChainManager> swapChain;
	extern std::unique_ptr<GraphicsQueueManager> graphicsQueue;
	extern std::unique_ptr<CommandListManager> graphicsCmdList;
	extern std::unique_ptr<DebugInfoManager> debugInfo;
	extern std::unique_ptr<DepthBuffer> depthBuffer;
	extern std::unique_ptr<ModelContainer> modelContainer;
	extern std::unique_ptr<CopyQueueManager> copyQueue;
	extern std::unique_ptr<CommandListManager> copyCmdList;
	extern std::unique_ptr<ResourceBuffer> vertexBuffer;
	extern std::unique_ptr<ResourceBuffer> indexBuffer;
	extern std::unique_ptr<ViewportAndScissorManager> viewportAndScissor;
	extern std::unique_ptr<HeapManager> heapManager;
	extern std::unique_ptr<DescriptorTableManager> descriptorTable;
	extern std::unique_ptr<TextureStorage> textureStorage;
	extern std::shared_ptr<IThreadPool> threadPool;
	extern std::unique_ptr<CPUAccessibleStorage> constantBuffer;
	extern std::unique_ptr<CameraManager> cameraManager;
	extern std::shared_ptr<ISharedDataContainer> sharedData;

	// Initialization functions
	void InitDevice();
	void InitSwapChain(const SwapChainCreateInfo& createInfo);
	void InitGraphicsQueue(ID3D12Device* d3dDevice, std::uint32_t syncObjCount);
	void InitGraphicsCmdList(ID3D12Device4* d3dDevice, std::uint32_t listCount);
	void InitDebugInfo();
	void InitDepthBuffer(ID3D12Device* d3dDevice);
	void InitModelContainer(const std::string& shaderPath);
	void InitCopyQueue(ID3D12Device* d3dDevice);
	void InitCopyCmdList(ID3D12Device4* d3dDevice);
	void InitVertexBuffer();
	void InitIndexBuffer();
	void InitViewportAndScissor(std::uint32_t width, std::uint32_t height);
	void InitHeapManager();
	void InitDescriptorTable();
	void InitTextureStorage();
	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg);
	void InitConstantBuffer();
	void InitCameraManager();
	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg);
}
#endif
