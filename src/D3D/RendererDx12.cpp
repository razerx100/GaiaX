#include <RendererDx12.hpp>
#include <Gaia.hpp>
#include <D3DHelperFunctions.hpp>
#include <PipelineConstructor.hpp>

RendererDx12::RendererDx12(
	const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount
) : m_backgroundColour{0.0001f, 0.0001f, 0.0001f, 0.0001f}, m_appName(appName),
	m_width(width), m_height(height), m_bufferCount{ bufferCount } {
	Gaia::InitDevice();
	Gaia::InitResources();

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
	Gaia::InitRenderPipeline(bufferCount);
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
	Gaia::renderPipeline.reset();
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
	Gaia::renderPipeline->RecordIndirectArguments(models);
	Gaia::bufferManager->AddOpaqueModels(std::move(models));
}

void RendererDx12::SubmitModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	Gaia::bufferManager->AddModelInputs(
		std::move(vertices), vertexBufferSize, strideSize,
		std::move(indices), indexBufferSize
	);
}

void RendererDx12::Update() {
	const size_t currentBackIndex = Gaia::swapChain->GetCurrentBackBufferIndex();

	Gaia::bufferManager->Update(currentBackIndex);
}

void RendererDx12::Render() {
	const size_t currentBackIndex = Gaia::swapChain->GetCurrentBackBufferIndex();

	// Compute Stage
	ID3D12GraphicsCommandList* computeCommandList = Gaia::computeCmdList->GetCommandList();
	Gaia::computeCmdList->Reset(currentBackIndex);
	// Need some barrier stuff

	ID3D12DescriptorHeap* ppHeap[] = { Gaia::descriptorTable->GetDescHeapRef() };
	computeCommandList->SetDescriptorHeaps(1u, ppHeap);

	// Record compute commands
	Gaia::renderPipeline->ResetCounterBuffer(computeCommandList, currentBackIndex);
	Gaia::renderPipeline->BindComputePipeline(computeCommandList);
	Gaia::bufferManager->BindBuffersToCompute(computeCommandList, currentBackIndex);
	Gaia::renderPipeline->DispatchCompute(computeCommandList, currentBackIndex);

	Gaia::computeCmdList->Close();
	Gaia::computeQueue->ExecuteCommandLists(computeCommandList);

	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

	Gaia::computeQueue->SignalCommandQueue(Gaia::computeFence->GetFence(), fenceValue);

	// Graphics Stage
	Gaia::graphicsQueue->WaitOnGPU(Gaia::computeFence->GetFence(), fenceValue);

	ID3D12GraphicsCommandList* graphicsCommandList = Gaia::graphicsCmdList->GetCommandList();

	Gaia::graphicsCmdList->Reset(currentBackIndex);

	RecordPreGraphicsBarriers(graphicsCommandList, currentBackIndex);

	graphicsCommandList->SetDescriptorHeaps(1u, ppHeap);

	graphicsCommandList->RSSetViewports(1u, Gaia::viewportAndScissor->GetViewportRef());
	graphicsCommandList->RSSetScissorRects(1u, Gaia::viewportAndScissor->GetScissorRef());

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = Gaia::swapChain->GetRTVHandle(currentBackIndex);

	Gaia::swapChain->ClearRTV(
		graphicsCommandList, std::data(m_backgroundColour), rtvHandle
	);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = Gaia::Resources::depthBuffer->GetDSVHandle();

	Gaia::Resources::depthBuffer->ClearDSV(graphicsCommandList, dsvHandle);

	graphicsCommandList->OMSetRenderTargets(1u, &rtvHandle, FALSE, &dsvHandle);

	// Record objects
	Gaia::renderPipeline->BindGraphicsPipeline(graphicsCommandList);
	Gaia::textureStorage->BindTextures(graphicsCommandList);
	Gaia::bufferManager->BindBuffersToGraphics(graphicsCommandList, currentBackIndex);
	Gaia::bufferManager->BindVertexBuffer(graphicsCommandList);
	Gaia::renderPipeline->DrawModels(graphicsCommandList, currentBackIndex);

	D3D12_RESOURCE_BARRIER presentBarrier = Gaia::swapChain->GetTransitionBarrier(
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, currentBackIndex
	);
	graphicsCommandList->ResourceBarrier(1u, &presentBarrier);

	Gaia::graphicsCmdList->Close();
	Gaia::graphicsQueue->ExecuteCommandLists(graphicsCommandList);

	Gaia::swapChain->PresentWithTear();

	fenceValue = Gaia::graphicsFence->GetFrontValue();

	Gaia::graphicsQueue->SignalCommandQueue(Gaia::graphicsFence->GetFence(), fenceValue);
	Gaia::graphicsFence->AdvanceValueInQueue();
	Gaia::graphicsFence->WaitOnCPUConditional();
	Gaia::graphicsFence->IncreaseFrontValue(fenceValue);
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
	m_backgroundColour = colour;
}

void RendererDx12::SetShaderPath(const wchar_t* path) noexcept {
	m_shaderPath = path;
}

void RendererDx12::ProcessData() {
	ID3D12Device* device = Gaia::device->GetDeviceRef();

	// Reserve Heap Space start
	Gaia::Resources::depthBuffer->ReserveHeapSpace(device);
	Gaia::renderPipeline->ReserveBuffers(device);
	Gaia::bufferManager->ReserveBuffers(device);
	Gaia::Resources::vertexBuffer->ReserveHeapSpace(device);
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
	Gaia::Resources::vertexBuffer->CreateResource(device);
	Gaia::renderPipeline->CreateBuffers(device);
	Gaia::bufferManager->CreateBuffers(device);
	Gaia::textureStorage->CreateBufferViews(device);
	// Create Buffers end

	// Set Buffer Start Address start
	std::uint8_t* vertexBufferUploadStartAddress =
		Gaia::Resources::vertexBuffer->GetCPUStartAddress();
	Gaia::Resources::vertexUploadContainer->SetStartingAddress(vertexBufferUploadStartAddress);
	// Set Buffer Start Address start

	// Async copy start
	std::atomic_size_t workCount = 0u;

	Gaia::Resources::vertexUploadContainer->CopyData(workCount);
	Gaia::Resources::textureUploadContainer->CopyData(workCount);

	Gaia::descriptorTable->CopyUploadHeap(device);

	while (workCount != 0u);
	// Async copy end

	// GPU upload start
	Gaia::copyCmdList->ResetFirst();
	ID3D12GraphicsCommandList* copyList = Gaia::copyCmdList->GetCommandList();

	Gaia::Resources::vertexBuffer->RecordResourceUpload(copyList);
	Gaia::renderPipeline->RecordResourceUpload(copyList);
	Gaia::textureStorage->RecordResourceUpload(copyList);

	Gaia::copyCmdList->Close();

	Gaia::copyQueue->ExecuteCommandLists(copyList);

	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();
	Gaia::copyQueue->SignalCommandQueue(Gaia::graphicsFence->GetFence(), fenceValue);
	Gaia::graphicsFence->WaitOnCPU();
	Gaia::graphicsFence->SignalFence(fenceValue - 1u);
	// GPU upload end

	ConstructPipelines();

	// Release Upload Resource start
	Gaia::renderPipeline->ReleaseUploadResource();
	Gaia::textureStorage->ReleaseUploadResource();
	Gaia::descriptorTable->ReleaseUploadHeap();
	Gaia::Resources::vertexBuffer->ReleaseUploadResource();
	Gaia::CleanUpUploadResources();
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

void RendererDx12::ConstructPipelines() {
	ID3D12Device* device = Gaia::device->GetDeviceRef();

	auto computeRS = CreateComputeRootSignature(device);
	auto graphicsRS = CreateGraphicsRootSignature(device);
	auto computePSO = CreateComputePipelineObject(device, m_shaderPath, computeRS->Get());
	auto graphicsPSO = CreateGraphicsPipelineObject(device, m_shaderPath, graphicsRS->Get());

	auto computeRSLayout = computeRS->GetElementLayout();
	auto graphicsRSLayout = graphicsRS->GetElementLayout();

	Gaia::bufferManager->SetComputeRootSignatureLayout(computeRSLayout);
	Gaia::renderPipeline->SetComputeRootSignatureLayout(std::move(computeRSLayout));
	Gaia::bufferManager->SetGraphicsRootSignatureLayout(graphicsRSLayout);
	Gaia::renderPipeline->SetGraphicsRootSignatureLayout(graphicsRSLayout);
	Gaia::textureStorage->SetGraphicsRootSignatureLayout(std::move(graphicsRSLayout));

	Gaia::renderPipeline->AddComputeRootSignature(std::move(computeRS));
	Gaia::renderPipeline->AddComputePipelineObject(std::move(computePSO));
	Gaia::renderPipeline->AddGraphicsRootSignature(std::move(graphicsRS));
	Gaia::renderPipeline->AddGraphicsPipelineObject(std::move(graphicsPSO));

	Gaia::renderPipeline->CreateCommandSignature(device);
}

void RendererDx12::RecordPreGraphicsBarriers(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) const noexcept {
	static D3D12_RESOURCE_BARRIER preBarriers[2]{};

	preBarriers[0] = Gaia::swapChain->GetTransitionBarrier(
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, frameIndex
	);
	preBarriers[1] = Gaia::renderPipeline->GetTransitionBarrier(
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
		frameIndex
	);
	graphicsCommandList->ResourceBarrier(2u, preBarriers);
}
