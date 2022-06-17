#include <RendererDx12.hpp>
#include <D3DThrowMacros.hpp>
#include <Gaia.hpp>
#include <D3DHelperFunctions.hpp>

RendererDx12::RendererDx12(
	const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount
) : m_backgroundColour{0.0001f, 0.0001f, 0.0001f, 0.0001f}, m_appName(appName),
	m_width(width), m_height(height) {
	Gaia::InitDevice();

#ifdef _DEBUG
	Gaia::InitDebugInfo();
#endif

	ID3D12Device4* deviceRef = Gaia::device.get()->GetDeviceRef();

	Gaia::InitDepthBuffer(deviceRef);
	Gaia::depthBuffer->CreateDepthBuffer(
		deviceRef, width, height
	);

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
	Gaia::InitVertexBuffer();
	Gaia::InitIndexBuffer();
	Gaia::InitConstantBuffer();
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
	Gaia::constantBuffer.reset();
	Gaia::vertexBuffer.reset();
	Gaia::indexBuffer.reset();
	Gaia::swapChain.reset();
	Gaia::graphicsCmdList.reset();
	Gaia::graphicsQueue.reset();
	Gaia::depthBuffer.reset();
	Gaia::heapManager.reset();
	Gaia::device.reset();
#ifdef _DEBUG
	Gaia::debugInfo.reset();
#endif
}

void RendererDx12::SubmitModel(std::shared_ptr<IModel> model) {
	Gaia::modelContainer->AddModel(std::move(model));
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

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = Gaia::depthBuffer->GetDSVHandle();

	Gaia::depthBuffer->ClearDSV(commandList, dsvHandle);

	commandList->OMSetRenderTargets(
		1u, &rtvHandle,
		FALSE, &dsvHandle
	);

	// Record objects
	Gaia::modelContainer->BindCommands(commandList);

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

		Gaia::depthBuffer->CreateDepthBuffer(
			deviceRef, width, height
		);
		Gaia::viewportAndScissor->Resize(width, height);

		Gaia::cameraManager->SetSceneResolution(width, height);
	}
}

Renderer::Resolution RendererDx12::GetDisplayCoordinates(std::uint32_t displayIndex) const {
	return GetDisplayResolution(
		Gaia::device->GetDeviceRef(), Gaia::device->GetFactoryRef(),
		displayIndex
	);
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
	Gaia::InitModelContainer(m_shaderPath);
}

void RendererDx12::ProcessData() {
	ID3D12Device* device = Gaia::device->GetDeviceRef();

	Gaia::descriptorTable->CreateDescriptorTable(device);

	Gaia::modelContainer->CreateBuffers(device);

	std::atomic_size_t workCount = 0u;

	Gaia::modelContainer->CopyData(workCount);
	Gaia::textureStorage->CopyData(workCount);

	while (workCount != 0u);

	Gaia::textureStorage->CreateBufferViews(device);

	Gaia::descriptorTable->CopyUploadHeap(device);

	Gaia::copyCmdList->Reset(0u);
	ID3D12GraphicsCommandList* copyList = Gaia::copyCmdList->GetCommandListRef();

	Gaia::modelContainer->RecordUploadBuffers(copyList);

	Gaia::copyCmdList->Close();

	Gaia::copyQueue->ExecuteCommandLists(copyList);
	Gaia::copyQueue->WaitForGPU();

	Gaia::modelContainer->InitPipelines(device);

	Gaia::textureStorage->ReleaseUploadBuffer();
	Gaia::descriptorTable->ReleaseUploadHeap();
	Gaia::modelContainer->ReleaseUploadBuffers();
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
