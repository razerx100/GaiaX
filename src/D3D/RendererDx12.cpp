#include <RendererDx12.hpp>
#include <Gaia.hpp>
#include <D3DHelperFunctions.hpp>
#include <D3DResourceBarrier.hpp>

RendererDx12::RendererDx12(
	const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
	RenderEngineType engineType
) : m_appName(appName), m_width(width), m_height(height), m_bufferCount{ bufferCount } {

	m_objectManager.CreateObject(Gaia::device, 3u);

	ID3D12Device4* deviceRef = Gaia::device.get()->GetDeviceRef();

	Gaia::InitRenderEngine(m_objectManager, engineType, deviceRef, bufferCount);
	Gaia::renderEngine->ResizeViewportAndScissor(width, height);

	Gaia::InitResources(m_objectManager);

#ifdef _DEBUG
	m_objectManager.CreateObject(Gaia::debugLogger, { deviceRef }, 4u);
#endif

	const bool meshDrawType = engineType == RenderEngineType::MeshDraw ? true : false;

	Gaia::InitGraphicsQueueAndList(m_objectManager, deviceRef, meshDrawType, bufferCount);

	SwapChainManager::Args swapChainArguments{
		.device = deviceRef,
		.factory = Gaia::device->GetFactoryRef(),
		.graphicsQueue = Gaia::graphicsQueue->GetQueue(),
		.windowHandle = static_cast<HWND>(windowHandle),
		.width = width,
		.height = height,
		.bufferCount = bufferCount,
		.variableRefreshRate = true
	};

	m_objectManager.CreateObject(Gaia::swapChain, swapChainArguments, 1u);

	Gaia::InitCopyQueueAndList(m_objectManager, deviceRef);
	Gaia::InitComputeQueueAndList(m_objectManager, deviceRef, bufferCount);

	m_objectManager.CreateObject(Gaia::descriptorTable, 0u);

	m_objectManager.CreateObject(Gaia::bufferManager, { bufferCount, meshDrawType }, 1u);
	m_objectManager.CreateObject(Gaia::textureStorage, 0u);

	m_objectManager.CreateObject(Gaia::cameraManager, 0u);
	Gaia::cameraManager->SetSceneResolution(width, height);
}

void RendererDx12::AddModelSet(
	std::vector<std::shared_ptr<IModel>>&& models, const std::wstring& pixelShader
) {
	Gaia::renderEngine->RecordModelDataSet(models, pixelShader + L".cso");
	Gaia::bufferManager->AddOpaqueModels(std::move(models));
}

void RendererDx12::AddMeshletModelSet(
	std::vector<MeshletModel>&& meshletModels, const std::wstring& pixelShader
) {
	Gaia::renderEngine->AddMeshletModelSet(std::move(meshletModels), pixelShader);
}

void RendererDx12::AddModelInputs(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
) {
	Gaia::renderEngine->AddGVerticesAndIndices(std::move(gVertices), std::move(gIndices));
}

void RendererDx12::AddModelInputs(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
	std::vector<std::uint32_t>&& gPrimIndices
) {
	Gaia::renderEngine->AddGVerticesAndPrimIndices(
		std::move(gVertices), std::move(gVerticesIndices), std::move(gPrimIndices)
	);
}

void RendererDx12::Update() {
	const size_t currentBackIndex = Gaia::swapChain->GetCurrentBackBufferIndex();

	Gaia::renderEngine->UpdateModelBuffers(currentBackIndex);
	Gaia::bufferManager->Update<false>(currentBackIndex);
}

void RendererDx12::Render() {
	const size_t currentBackIndex = Gaia::swapChain->GetCurrentBackBufferIndex();

	Gaia::renderEngine->ExecuteRenderStage(currentBackIndex);
	Gaia::renderEngine->Present(currentBackIndex);
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

		Gaia::renderEngine->CreateDepthBufferView(deviceRef, width, height);
		Gaia::renderEngine->ResizeViewportAndScissor(width, height);

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
	Gaia::renderEngine->ReserveBuffers(device);
	Gaia::bufferManager->ReserveBuffers(device);
	Gaia::Resources::cpuWriteBuffer->ReserveHeapSpace(device);
	// Reserve Heap Space end

	// Create heaps start
	Gaia::Resources::uploadHeap->CreateHeap(device);
	Gaia::Resources::gpuOnlyHeap->CreateHeap(device);
	Gaia::Resources::cpuWriteHeap->CreateHeap(device);
	// Create heaps end

	Gaia::descriptorTable->CreateDescriptorTable(device);

	// Create Buffers start
	Gaia::renderEngine->CreateDepthBufferView(device, m_width, m_height);
	Gaia::Resources::cpuWriteBuffer->CreateResource(device);
	Gaia::renderEngine->CreateBuffers(device);
	Gaia::bufferManager->CreateBuffers(device);
	Gaia::textureStorage->CreateBufferViews(device);
	// Create Buffers end

	// Async copy start
	std::atomic_size_t workCount = 0u;

	Gaia::renderEngine->CopyData(workCount);
	Gaia::textureStorage->CopyData(workCount);

	Gaia::descriptorTable->CopyUploadHeap(device);

	while (workCount != 0u);
	// Async copy end

	// GPU upload start
	Gaia::copyCmdList->ResetFirst();
	ID3D12GraphicsCommandList* copyList = Gaia::copyCmdList->GetCommandList();

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
	Gaia::Resources::uploadHeap.reset();
	// Release Upload Resource end
}

size_t RendererDx12::AddTexture(
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
