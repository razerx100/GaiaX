#include <RendererDx12.hpp>
#include <D3DThrowMacros.hpp>
#include <Gaia.hpp>
#include <D3DHelperFunctions.hpp>

RendererDx12::RendererDx12(
	const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount
) : m_backgroundColour{0.0001f, 0.0001f, 0.0001f, 0.0001f}, m_appName(appName),
	m_width(width), m_height(height), m_bufferCount{ bufferCount } {
	Gaia::InitDevice();
	Gaia::InitResources();

#ifdef _DEBUG
	Gaia::InitDebugInfo();
#endif

	ID3D12Device4* deviceRef = Gaia::device.get()->GetDeviceRef();

	Gaia::InitDepthBuffer(deviceRef);
	Gaia::Resources::depthBuffer->SetMaxResolution(7680u, 4320u);

	Gaia::InitGraphicsQueue(deviceRef, bufferCount);
	Gaia::InitGraphicsCmdList(deviceRef, bufferCount);

	SwapChainCreateInfo swapChainCreateInfo = {};
	swapChainCreateInfo.bufferCount = bufferCount;
	swapChainCreateInfo.width = width;
	swapChainCreateInfo.height = height;
	swapChainCreateInfo.device = deviceRef;
	swapChainCreateInfo.factory = Gaia::device->GetFactoryRef();
	swapChainCreateInfo.graphicsQueue = Gaia::graphicsQueue->GetQueueRef();
	swapChainCreateInfo.windowHandle = static_cast<HWND>(windowHandle);
	swapChainCreateInfo.variableRefreshRate = true;

	Gaia::InitSwapChain(swapChainCreateInfo);

	Gaia::graphicsQueue->InitSyncObjects(
		deviceRef,
		Gaia::swapChain->GetCurrentBackBufferIndex()
	);

	Gaia::InitViewportAndScissor(width, height);

	Gaia::InitCopyQueue(deviceRef);
	Gaia::copyQueue->InitSyncObjects(deviceRef);

	Gaia::InitCopyCmdList(deviceRef);
	Gaia::InitHeapManager();
	Gaia::InitDescriptorTable();
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
	Gaia::modelContainer.reset();
	Gaia::swapChain.reset();
	Gaia::graphicsCmdList.reset();
	Gaia::graphicsQueue.reset();
	Gaia::heapManager.reset();
	Gaia::CleanUpResources();
	Gaia::device.reset();
#ifdef _DEBUG
	Gaia::debugInfo.reset();
#endif
}

void RendererDx12::SubmitModels(std::vector<std::shared_ptr<IModel>>&& models) {
	Gaia::modelContainer->AddModels(std::move(models));
}

void RendererDx12::SubmitModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	Gaia::modelContainer->AddModelInputs(
		std::move(vertices), vertexBufferSize, strideSize,
		std::move(indices), indexBufferSize
	);
}

void RendererDx12::Render() {
	ID3D12GraphicsCommandList* commandList = Gaia::graphicsCmdList->GetCommandListRef();

	size_t currentBackIndex = Gaia::swapChain->GetCurrentBackBufferIndex();

	Gaia::graphicsCmdList->Reset(currentBackIndex);
	D3D12_RESOURCE_BARRIER renderBarrier = Gaia::swapChain->GetRenderStateBarrier(
		currentBackIndex
	);
	commandList->ResourceBarrier(1u, &renderBarrier);

	ID3D12DescriptorHeap* ppHeap[] = { Gaia::descriptorTable->GetDescHeapRef() };
	commandList->SetDescriptorHeaps(1u, ppHeap);

	commandList->RSSetViewports(
		1u, Gaia::viewportAndScissor->GetViewportRef()
	);
	commandList->RSSetScissorRects(
		1u, Gaia::viewportAndScissor->GetScissorRef()
	);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = Gaia::swapChain->GetRTVHandle(currentBackIndex);

	Gaia::swapChain->ClearRTV(
		commandList, std::data(m_backgroundColour), rtvHandle
	);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = Gaia::Resources::depthBuffer->GetDSVHandle();

	Gaia::Resources::depthBuffer->ClearDSV(commandList, dsvHandle);

	commandList->OMSetRenderTargets(
		1u, &rtvHandle, FALSE, &dsvHandle
	);

	// Record objects
	Gaia::modelContainer->BindCommands(commandList, currentBackIndex);

	D3D12_RESOURCE_BARRIER presentBarrier = Gaia::swapChain->GetPresentStateBarrier(
		currentBackIndex
	);
	commandList->ResourceBarrier(1u, &presentBarrier);

	Gaia::graphicsCmdList->Close();
	Gaia::graphicsQueue->ExecuteCommandLists(commandList);

	Gaia::swapChain->PresentWithTear();
	Gaia::graphicsQueue->MoveToNextFrame(currentBackIndex);
}

void RendererDx12::Resize(std::uint32_t width, std::uint32_t height) {
	if (m_width != width || m_height != height) {
		m_width = width;
		m_height = height;

		ID3D12Device* deviceRef = Gaia::device->GetDeviceRef();

		size_t backBufferIndex = Gaia::swapChain->GetCurrentBackBufferIndex();

		Gaia::graphicsQueue->WaitForGPU(
			backBufferIndex
		);

		Gaia::swapChain->Resize(deviceRef, width, height);
		Gaia::graphicsQueue->ResetFenceValuesWith(
			backBufferIndex
		);

		Gaia::Resources::depthBuffer->CreateDepthBuffer(
			deviceRef, width, height
		);
		Gaia::viewportAndScissor->Resize(width, height);

		Gaia::cameraManager->SetSceneResolution(width, height);
	}
}

Renderer::Resolution RendererDx12::GetDisplayCoordinates(std::uint32_t displayIndex) const {
	auto [width, height] = GetDisplayResolution(
		Gaia::device->GetDeviceRef(), Gaia::device->GetFactoryRef(), displayIndex
	);

	return { width, height };
}

void RendererDx12::SetBackgroundColour(const std::array<float, 4>& colour) noexcept {
	m_backgroundColour = colour;
}

void RendererDx12::WaitForAsyncTasks() {
	Gaia::graphicsQueue->WaitForGPU(
		Gaia::swapChain->GetCurrentBackBufferIndex()
	);
	Gaia::copyQueue->WaitForGPU();
}

void RendererDx12::SetShaderPath(const char* path) noexcept {
	m_shaderPath = path;
}

void RendererDx12::InitResourceBasedObjects() {
	Gaia::InitModelContainer(m_shaderPath, m_bufferCount);
}

void RendererDx12::ProcessData() {
	ID3D12Device* device = Gaia::device->GetDeviceRef();

	// Reserve Heap Space start
	Gaia::Resources::depthBuffer->ReserveHeapSpace(device);
	Gaia::modelContainer->ReserveBuffers(device);
	Gaia::Resources::vertexBuffer->ReserveHeapSpace(device);
	Gaia::Resources::cpuWriteBuffer->ReserveHeapSpace(device);

	Gaia::heapManager->ReserveHeapSpace();
	// Reserve Heap Space end

	// Create heaps start
	Gaia::Resources::uploadHeap->CreateHeap(device);
	Gaia::Resources::gpuOnlyHeap->CreateHeap(device);
	Gaia::Resources::cpuWriteHeap->CreateHeap(device);
	// Create heaps end

	Gaia::descriptorTable->CreateDescriptorTable(device);

	// Create Buffers start
	Gaia::Resources::depthBuffer->CreateDepthBuffer(device, m_width, m_height);
	Gaia::heapManager->CreateBuffers(device);
	Gaia::Resources::cpuWriteBuffer->CreateResource(device);
	Gaia::Resources::vertexBuffer->CreateResource(device);
	Gaia::modelContainer->CreateBuffers(device);
	// Create Buffers end

	// Set Buffer Start Address start
	std::uint8_t* vertexBufferUploadStartAddress =
		Gaia::Resources::vertexBuffer->GetCPUStartAddress();
	Gaia::Resources::vertexUploadContainer->SetStartingAddress(vertexBufferUploadStartAddress);
	// Set Buffer Start Address start

	// Async copy start
	std::atomic_size_t workCount = 0u;

	Gaia::Resources::vertexUploadContainer->CopyData(workCount);
	Gaia::textureStorage->CopyData(workCount);

	while (workCount != 0u);
	// Async copy end

	Gaia::textureStorage->CreateBufferViews(device);

	Gaia::descriptorTable->CopyUploadHeap(device);

	// GPU upload start
	Gaia::copyCmdList->Reset(0u);
	ID3D12GraphicsCommandList* copyList = Gaia::copyCmdList->GetCommandListRef();

	Gaia::Resources::vertexBuffer->RecordResourceUpload(copyList);
	Gaia::heapManager->RecordUpload(copyList);

	Gaia::copyCmdList->Close();

	Gaia::copyQueue->ExecuteCommandLists(copyList);
	Gaia::copyQueue->WaitForGPU();
	// GPU upload end

	Gaia::modelContainer->InitPipelines(device);

	// Release Upload Resource start
	Gaia::textureStorage->ReleaseUploadBuffer();
	Gaia::descriptorTable->ReleaseUploadHeap();
	Gaia::Resources::vertexBuffer->ReleaseUploadResource();
	Gaia::heapManager->ReleaseUploadBuffer();
	Gaia::CleanUpUploadResources();
	Gaia::Resources::uploadHeap.reset();
	// Release Upload Resource end
}

size_t RendererDx12::RegisterResource(
	std::unique_ptr<std::uint8_t> textureData,
	size_t width, size_t height, bool components16bits
) {
	return Gaia::textureStorage->AddTexture(
		Gaia::device->GetDeviceRef(), std::move(textureData),
		width, height, components16bits
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
