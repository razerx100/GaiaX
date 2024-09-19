#include <Gaia.hpp>
#include <RenderEngineVertexShader.hpp>
#include <RenderEngineMeshShader.hpp>

namespace Gaia {
	std::unique_ptr<DeviceManager> device;
	//std::unique_ptr<SwapChainManager> swapChain;
	std::unique_ptr<D3DCommandQueue> graphicsQueue;
	std::unique_ptr<D3DCommandList> graphicsCmdList;
	std::unique_ptr<D3DFence> graphicsFence;
	std::unique_ptr<D3DDebugLogger> debugLogger;
	//std::unique_ptr<BufferManager> bufferManager;
	std::unique_ptr<D3DCommandQueue> copyQueue;
	std::unique_ptr<D3DCommandList> copyCmdList;
	//std::unique_ptr<DescriptorTableManager> descriptorTable;
	//std::unique_ptr<TextureStorage> textureStorage;
	std::unique_ptr<CameraManager> cameraManager;
	std::unique_ptr<D3DCommandQueue> computeQueue;
	std::unique_ptr<D3DCommandList> computeCmdList;
	std::unique_ptr<D3DFence> computeFence;
	std::unique_ptr<RenderEngine> renderEngine;

	namespace Resources {
		std::unique_ptr<D3DHeap> uploadHeap;
		std::unique_ptr<D3DHeap> cpuWriteHeap;
		std::unique_ptr<D3DHeap> gpuOnlyHeap;
		std::unique_ptr<D3DHeap> cpuReadBackHeap;
		//std::unique_ptr<D3DResourceBuffer> cpuWriteBuffer;
		//std::unique_ptr<UploadContainer> uploadContainer;
	}

	void InitGraphicsQueueAndList(
		ObjectManager& om, ID3D12Device4* d3dDevice, bool cmdList6,
		std::uint32_t commandAllocatorCount
	) {
		/*
		om.CreateObject(graphicsQueue, 1u, d3dDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		om.CreateObject(
			graphicsCmdList, 1u,
			d3dDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdList6, commandAllocatorCount
		);
		om.CreateObject(graphicsFence, 1u, d3dDevice, commandAllocatorCount);
		*/
	}

	void InitCopyQueueAndList(ObjectManager& om, ID3D12Device4* d3dDevice) {
		/*
		om.CreateObject(copyQueue, 1u, d3dDevice, D3D12_COMMAND_LIST_TYPE_COPY);
		om.CreateObject(
			copyCmdList, 1u, d3dDevice, D3D12_COMMAND_LIST_TYPE_COPY, false
		);
		*/
	}

	void InitComputeQueueAndList(
		ObjectManager& om, ID3D12Device4* d3dDevice, std::uint32_t commandAllocatorCount
	) {
		/*
		om.CreateObject(computeQueue, 1u, d3dDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		om.CreateObject(
			computeCmdList, 1u,
			d3dDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE, false, commandAllocatorCount
		);
		om.CreateObject(computeFence, 1u, d3dDevice, commandAllocatorCount);
		*/
	}

	void InitRenderEngine(
		ObjectManager& om, RenderEngineType engineType, ID3D12Device* d3dDevice,
		std::uint32_t frameCount
	) {
		/*
		switch (engineType)
		{
		case RenderEngineType::IndirectDraw: {
			om.CreateObject<RenderEngine, RenderEngineIndirectDraw>(
				renderEngine, 1u, d3dDevice, frameCount
			);
			break;
		}
		case RenderEngineType::IndividualDraw: {
			om.CreateObject<RenderEngine, RenderEngineIndividualDraw>(renderEngine, 1u, d3dDevice);
			break;
		}
		case RenderEngineType::MeshDraw: {
			om.CreateObject<RenderEngine, RenderEngineMeshDraw>(renderEngine, 1u, d3dDevice);
			break;
		}
		}
		*/
	}

	void InitResources(ObjectManager& om, ThreadPool& threadPooll) {
		/*
		om.CreateObject(Resources::uploadHeap, 0u, D3D12_HEAP_TYPE_UPLOAD );
		om.CreateObject(Resources::cpuWriteHeap, 2u, D3D12_HEAP_TYPE_UPLOAD);
		om.CreateObject(Resources::cpuReadBackHeap, 2u, D3D12_HEAP_TYPE_READBACK);
		om.CreateObject(Resources::gpuOnlyHeap, 2u, D3D12_HEAP_TYPE_DEFAULT);
		*/

		//om.CreateObject(Resources::cpuWriteBuffer, 1u, ResourceType::cpuWrite);
		//om.CreateObject(Resources::uploadContainer, 0u, threadPooll);
	}
}
