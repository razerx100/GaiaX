#include <Gaia.hpp>
#include <RenderEngineVertexShader.hpp>
#include <RenderEngineMeshShader.hpp>

namespace Gaia {
	std::unique_ptr<DeviceManager> device;
	std::unique_ptr<SwapChainManager> swapChain;
	std::unique_ptr<D3DCommandQueue> graphicsQueue;
	std::unique_ptr<D3DCommandList> graphicsCmdList;
	std::unique_ptr<D3DFence> graphicsFence;
	std::unique_ptr<D3DDebugLogger> debugLogger;
	std::unique_ptr<BufferManager> bufferManager;
	std::unique_ptr<D3DCommandQueue> copyQueue;
	std::unique_ptr<D3DCommandList> copyCmdList;
	std::unique_ptr<DescriptorTableManager> descriptorTable;
	std::unique_ptr<TextureStorage> textureStorage;
	std::shared_ptr<IThreadPool> threadPool;
	std::unique_ptr<CameraManager> cameraManager;
	std::shared_ptr<ISharedDataContainer> sharedData;
	std::unique_ptr<D3DCommandQueue> computeQueue;
	std::unique_ptr<D3DCommandList> computeCmdList;
	std::unique_ptr<D3DFence> computeFence;
	std::unique_ptr<RenderEngine> renderEngine;

	namespace Resources {
		std::unique_ptr<D3DHeap> uploadHeap;
		std::unique_ptr<D3DHeap> cpuWriteHeap;
		std::unique_ptr<D3DHeap> gpuOnlyHeap;
		std::unique_ptr<D3DHeap> cpuReadBackHeap;
		std::unique_ptr<D3DResourceBuffer> cpuWriteBuffer;
		std::unique_ptr<UploadContainer> uploadContainer;
	}

	void InitGraphicsQueueAndList(
		ObjectManager& om, ID3D12Device4* d3dDevice, bool cmdList6,
		std::uint32_t commandAllocatorCount
	) {
		om.CreateObject(graphicsQueue, { d3dDevice, D3D12_COMMAND_LIST_TYPE_DIRECT }, 1u);
		om.CreateObject(
			graphicsCmdList,
			{ d3dDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdList6, commandAllocatorCount }, 1u
		);
		om.CreateObject(graphicsFence, { d3dDevice, commandAllocatorCount }, 1u);
	}

	void InitCopyQueueAndList(ObjectManager& om, ID3D12Device4* d3dDevice) {
		om.CreateObject(copyQueue, { d3dDevice, D3D12_COMMAND_LIST_TYPE_COPY }, 1u);
		om.CreateObject(
			copyCmdList,
			{ .device = d3dDevice, .type = D3D12_COMMAND_LIST_TYPE_COPY, .cmdList6 = false },
			1u
		);
	}

	void InitComputeQueueAndList(
		ObjectManager& om, ID3D12Device4* d3dDevice, std::uint32_t commandAllocatorCount
	) {
		om.CreateObject(computeQueue, { d3dDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE }, 1u);
		om.CreateObject(
			computeCmdList,
			{ d3dDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE, false, commandAllocatorCount }, 1u
		);
		om.CreateObject(computeFence, { d3dDevice, commandAllocatorCount }, 1u);
	}

	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg) {
		threadPool = std::move(threadPoolArg);
	}

	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg) {
		sharedData = std::move(sharedDataArg);
	}

	void InitRenderEngine(
		ObjectManager& om, RenderEngineType engineType, ID3D12Device* d3dDevice,
		std::uint32_t frameCount
	) {
		switch (engineType) {
		case RenderEngineType::IndirectDraw: {
			om.CreateObject<RenderEngineIndirectDraw>(
				renderEngine, { d3dDevice, frameCount }, 1u
			);
			break;
		}
		case RenderEngineType::IndividualDraw: {
			om.CreateObject<RenderEngineIndividualDraw>(renderEngine, { d3dDevice }, 1u);
			break;
		}
		case RenderEngineType::MeshDraw: {
			om.CreateObject<RenderEngineMeshDraw>(renderEngine, { d3dDevice }, 1u);
			break;
		}
		}
	}

	void InitResources(ObjectManager& om) {
		om.CreateObject(Resources::uploadHeap, { D3D12_HEAP_TYPE_UPLOAD }, 0u);
		om.CreateObject(Resources::cpuWriteHeap, { D3D12_HEAP_TYPE_UPLOAD }, 2u);
		om.CreateObject(Resources::cpuReadBackHeap, { D3D12_HEAP_TYPE_READBACK }, 2u);
		om.CreateObject(Resources::gpuOnlyHeap, { D3D12_HEAP_TYPE_DEFAULT }, 2u);

		om.CreateObject(Resources::cpuWriteBuffer, { ResourceType::cpuWrite }, 1u);
		om.CreateObject(Resources::uploadContainer, 0u);
	}
}
