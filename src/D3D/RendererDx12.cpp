#include <RendererDx12.hpp>
#include <Gaia.hpp>
#include <D3DHelperFunctions.hpp>
#include <D3DResourceBarrier.hpp>

RendererDx12::RendererDx12(
	const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount
) : m_appName(appName), m_width(width), m_height(height), m_bufferCount{ bufferCount } {
	Gaia::InitDevice();
	Gaia::InitRenderEngine();
	Gaia::InitResources();
	Gaia::InitVertexManager();

	ID3D12Device4* deviceRef = Gaia::device.get()->GetDeviceRef();

#ifdef _DEBUG
	Gaia::InitDebugLogger(deviceRef);
#endif

	Gaia::InitDepthBuffer(deviceRef);
	Gaia::Resources::depthBuffer->SetMaxResolution(7680u, 4320u);

	Gaia::InitGraphicsQueueAndList(deviceRef, bufferCount);

	SwapChainCreateInfo swapChainCreateInfo = {};
	swapChainCreateInfo.bufferCount = bufferCount;
	swapChainCreateInfo.width = width;
	swapChainCreateInfo.height = height;
	swapChainCreateInfo.device = deviceRef;
	swapChainCreateInfo.factory = Gaia::device->GetFactoryRef();
	swapChainCreateInfo.graphicsQueue = Gaia::graphicsQueue->GetQueue();
	swapChainCreateInfo.windowHandle = static_cast<HWND>(windowHandle);
	swapChainCreateInfo.variableRefreshRate = true;

	Gaia::InitSwapChain(swapChainCreateInfo);
	Gaia::InitViewportAndScissor(width, height);

	Gaia::InitCopyQueueAndList(deviceRef);
	Gaia::InitComputeQueueAndList(deviceRef, bufferCount);

	Gaia::InitDescriptorTable();
	Gaia::renderEngine->InitiatePipelines(bufferCount);
	Gaia::InitBufferManager(bufferCount);
	Gaia::InitTextureStorage();

	Gaia::InitCameraManager();
	Gaia::cameraManager->SetSceneResolution(width, height);
}

RendererDx12::~RendererDx12() noexcept {
	Gaia::cameraManager.reset();
	Gaia::textureStorage.reset();
	Gaia::descriptorTable.reset();
	Gaia::viewportAndScissor.reset();
	Gaia::copyCmdList.reset();
	Gaia::copyQueue.reset();
	Gaia::renderEngine.reset();
	Gaia::vertexManager.reset();
	Gaia::bufferManager.reset();
	Gaia::computeFence.reset();
	Gaia::computeCmdList.reset();
	Gaia::computeQueue.reset();
	Gaia::swapChain.reset();
	Gaia::graphicsFence.reset();
	Gaia::graphicsCmdList.reset();
	Gaia::graphicsQueue.reset();
	Gaia::CleanUpResources();
	Gaia::device.reset();
#ifdef _DEBUG
	Gaia::debugLogger.reset();
#endif
}

void RendererDx12::SubmitModels(std::vector<std::shared_ptr<IModel>>&& models) {
	Gaia::renderEngine->RecordModelData(models);
	Gaia::bufferManager->AddOpaqueModels(std::move(models));
}

void RendererDx12::SubmitModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	Gaia::vertexManager->AddGlobalVertices(
		std::move(vertices), vertexBufferSize, strideSize, std::move(indices), indexBufferSize
	);
}

void RendererDx12::Update() {
	const size_t currentBackIndex = Gaia::swapChain->GetCurrentBackBufferIndex();

	Gaia::bufferManager->Update(currentBackIndex);
}

void RendererDx12::Render() {
	const size_t currentBackIndex = Gaia::swapChain->GetCurrentBackBufferIndex();
	ID3D12GraphicsCommandList* graphicsCommandList = Gaia::graphicsCmdList->GetCommandList();

	Gaia::renderEngine->ExecutePreRenderStage(graphicsCommandList, currentBackIndex);
	Gaia::renderEngine->RecordDrawCommands(graphicsCommandList, currentBackIndex);
	Gaia::renderEngine->Present(graphicsCommandList, currentBackIndex);
	Gaia::renderEngine->ExecutePostRenderStage();
}

void RendererDx12::Resize(std::uint32_t width, std::uint32_t height) {
	if (m_width != width || m_height != height) {
		m_width = width;
		m_height = height;

		ID3D12Device* deviceRef = Gaia::device->GetDeviceRef();

		UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

		Gaia::graphicsQueue->SignalCommandQueue(Gaia::graphicsFence->GetFence(), fenceValue);
		Gaia::graphicsFence->WaitOnCPU();

		Gaia::swapChain->Resize(deviceRef, width, height);

		Gaia::graphicsFence->ResetFenceValues(fenceValue + 1u);

		Gaia::Resources::depthBuffer->CreateDepthBuffer(
			deviceRef, width, height
		);
		Gaia::viewportAndScissor->Resize(width, height);

		Gaia::cameraManager->SetSceneResolution(width, height);
	}
}

Renderer::Resolution RendererDx12::GetFirstDisplayCoordinates() const {
	auto [width, height] = GetDisplayResolution(
		Gaia::device->GetDeviceRef(), Gaia::device->GetFactoryRef(), 0u
	);

	return { width, height };
}

void RendererDx12::SetBackgroundColour(const std::array<float, 4>& colour) noexcept {
	Gaia::renderEngine->SetBackgroundColour(colour);
}

void RendererDx12::SetShaderPath(const wchar_t* path) noexcept {
	Gaia::renderEngine->SetShaderPath(path);
}

void RendererDx12::ProcessData() {
	ID3D12Device* device = Gaia::device->GetDeviceRef();

	// Reserve Heap Space start
	Gaia::Resources::depthBuffer->ReserveHeapSpace(device);
	Gaia::renderEngine->ReserveBuffers(device);
	Gaia::bufferManager->ReserveBuffers(device);
	Gaia::vertexManager->ReserveBuffers(device);
	Gaia::Resources::cpuWriteBuffer->ReserveHeapSpace(device);
	// Reserve Heap Space end

	// Create heaps start
	Gaia::Resources::uploadHeap->CreateHeap(device);
	Gaia::Resources::gpuOnlyHeap->CreateHeap(device);
	Gaia::Resources::cpuWriteHeap->CreateHeap(device);
	// Create heaps end

	Gaia::descriptorTable->CreateDescriptorTable(device);

	// Create Buffers start
	Gaia::Resources::depthBuffer->CreateDepthBuffer(device, m_width, m_height);
	Gaia::Resources::cpuWriteBuffer->CreateResource(device);
	Gaia::vertexManager->CreateBuffers(device);
	Gaia::renderEngine->CreateBuffers(device);
	Gaia::bufferManager->CreateBuffers(device);
	Gaia::textureStorage->CreateBufferViews(device);
	// Create Buffers end

	// Async copy start
	std::atomic_size_t workCount = 0u;

	Gaia::vertexManager->CopyData(workCount);
	Gaia::textureStorage->CopyData(workCount);

	Gaia::descriptorTable->CopyUploadHeap(device);

	while (workCount != 0u);
	// Async copy end

	// GPU upload start
	Gaia::copyCmdList->ResetFirst();
	ID3D12GraphicsCommandList* copyList = Gaia::copyCmdList->GetCommandList();

	Gaia::vertexManager->RecordResourceUploads(copyList);
	Gaia::renderEngine->RecordResourceUploads(copyList);
	Gaia::textureStorage->RecordResourceUpload(copyList);

	Gaia::copyCmdList->Close();

	Gaia::copyQueue->ExecuteCommandLists(copyList);

	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();
	Gaia::copyQueue->SignalCommandQueue(Gaia::graphicsFence->GetFence(), fenceValue);
	Gaia::graphicsFence->WaitOnCPU();
	Gaia::graphicsFence->SignalFence(fenceValue - 1u);
	// GPU upload end

	Gaia::renderEngine->ConstructPipelines();

	// Release Upload Resource start
	Gaia::renderEngine->ReleaseUploadResources();
	Gaia::textureStorage->ReleaseUploadResource();
	Gaia::descriptorTable->ReleaseUploadHeap();
	Gaia::vertexManager->ReleaseUploadResources();
	Gaia::Resources::uploadHeap.reset();
	// Release Upload Resource end
}

size_t RendererDx12::RegisterResource(
	std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
) {
	return Gaia::textureStorage->AddTexture(
		Gaia::device->GetDeviceRef(), std::move(textureData), width, height
	);
}

void RendererDx12::SetThreadPool(std::shared_ptr<IThreadPool> threadPoolArg) noexcept {
	Gaia::SetThreadPool(std::move(threadPoolArg));
}

void RendererDx12::SetSharedDataContainer(
	std::shared_ptr<ISharedDataContainer> sharedData
) noexcept {
	Gaia::SetSharedData(std::move(sharedData));
}

void RendererDx12::WaitForAsyncTasks() {
	// Current frame's value is already checked. So, check the rest
	for (std::uint32_t _ = 0u; _ < m_bufferCount - 1u; ++_) {
		Gaia::graphicsFence->AdvanceValueInQueue();
		Gaia::graphicsFence->WaitOnCPUConditional();
		Gaia::computeFence->AdvanceValueInQueue();
		Gaia::computeFence->WaitOnCPUConditional();
	}
}
