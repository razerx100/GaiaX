#include <GraphicsEngineDx12.hpp>
#include <D3DThrowMacros.hpp>
#include <InstanceManager.hpp>

GraphicsEngineDx12::GraphicsEngineDx12(
	const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) : m_backgroundColor{0.1f, 0.1f, 0.1f, 0.1f}, m_appName(appName) {
	DeviceInst::Init();

#ifdef _DEBUG
	DebugInfoInst::Init();
#endif

	ID3D12Device5* deviceRef = DeviceInst::GetRef()->GetDeviceRef();

	DepthBuffInst::Init(
		deviceRef
	);
	DepthBuffInst::GetRef()->CreateDepthBuffer(
		deviceRef, width, height
	);

	GfxQueInst::Init(
		deviceRef,
		bufferCount
	);
	GfxCmdListInst::Init(
		deviceRef,
		bufferCount
	);

	IGraphicsQueueManager* queueRef = GfxQueInst::GetRef();

	SwapchainInst::Init(
		DeviceInst::GetRef()->GetFactoryRef(),
		queueRef->GetQueueRef(),
		windowHandle,
		bufferCount, width, height
	);

	queueRef->InitSyncObjects(
		deviceRef,
		SwapchainInst::GetRef()->GetCurrentBackBufferIndex()
	);

	InitViewPortAndScissor(width, height);

	queueRef->WaitForGPU(
		SwapchainInst::GetRef()->GetCurrentBackBufferIndex()
	);
}

GraphicsEngineDx12::~GraphicsEngineDx12() noexcept {
	ModelContainerInst::CleanUp();
	SwapchainInst::CleanUp();
	GfxCmdListInst::CleanUp();
	GfxQueInst::CleanUp();
	DepthBuffInst::CleanUp();
	DeviceInst::CleanUp();
#ifdef _DEBUG
	DebugInfoInst::CleanUp();
#endif
}

void GraphicsEngineDx12::SubmitModels(
	IModel* models, std::uint32_t modelCount,
	bool texture
) {
	if (texture)
		for (std::uint32_t index = 0u; index < modelCount; ++index)
			ModelContainerInst::GetRef()->AddTexturedModel(
				DeviceInst::GetRef()->GetDeviceRef(),
				std::unique_ptr<IModel>(models + index)
			);
	else
		for (std::uint32_t index = 0u; index < modelCount; ++index)
			ModelContainerInst::GetRef()->AddColoredModel(
				DeviceInst::GetRef()->GetDeviceRef(),
				std::unique_ptr<IModel>(models + index)
			);
}

void GraphicsEngineDx12::Render() {
	IGraphicsQueueManager* queueRef = GfxQueInst::GetRef();
	ISwapChainManager* swapRef = SwapchainInst::GetRef ();
	ICommandListManager* listManRef = GfxCmdListInst::GetRef();
	ID3D12GraphicsCommandList* commandList = listManRef->GetCommandListRef();

	listManRef->Reset(
		swapRef->GetCurrentBackBufferIndex()
	);
	D3D12_RESOURCE_BARRIER renderBarrier = swapRef->GetRenderStateBarrier();
	commandList->ResourceBarrier(1, &renderBarrier);

	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapRef->GetRTVHandle();

	swapRef->ClearRTV(
		commandList, &m_backgroundColor.F32.x, rtvHandle
	);

	IDepthBuffer* depthRef = DepthBuffInst::GetRef();

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthRef->GetDSVHandle();

	depthRef->ClearDSV(commandList, dsvHandle);

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Record objects
	ModelContainerInst::GetRef()->BindCommands(commandList);

	D3D12_RESOURCE_BARRIER presentBarrier = swapRef->GetPresentStateBarrier();
	commandList->ResourceBarrier(1, &presentBarrier);

	listManRef->Close();
	queueRef->ExecuteCommandLists(commandList);

	std::uint32_t backBufferIndex = swapRef->GetCurrentBackBufferIndex();

	swapRef->PresentWithTear();
	queueRef->MoveToNextFrame(backBufferIndex);
}

void GraphicsEngineDx12::Resize(std::uint32_t width, std::uint32_t height) {
	ISwapChainManager* swapRef = SwapchainInst::GetRef();
	GfxQueInst::GetRef()->WaitForGPU(
		swapRef->GetCurrentBackBufferIndex()
	);
	swapRef->Resize(width, height);
	DepthBuffInst::GetRef()->CreateDepthBuffer(
		DeviceInst::GetRef()->GetDeviceRef(),
		width, height
	);
	InitViewPortAndScissor(width, height);
}

void GraphicsEngineDx12::GetMonitorCoordinates(
	std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
) {
	ComPtr<IDXGIOutput> pOutput;
	HRESULT hr;
	D3D_THROW_FAILED(hr,
		SwapchainInst::GetRef()->GetRef()->GetContainingOutput(&pOutput)
	);

	DXGI_OUTPUT_DESC desc;
	D3D_THROW_FAILED(hr, pOutput->GetDesc(&desc));

	monitorWidth = static_cast<std::uint64_t>(desc.DesktopCoordinates.right);
	monitorHeight = static_cast<std::uint64_t>(desc.DesktopCoordinates.bottom);
}

void  GraphicsEngineDx12::InitViewPortAndScissor(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.left = static_cast<LONG>(m_viewport.TopLeftX);
	m_scissorRect.right = static_cast<LONG>(m_viewport.TopLeftX + m_viewport.Width);
	m_scissorRect.top = static_cast<LONG>(m_viewport.TopLeftY);
	m_scissorRect.bottom = static_cast<LONG>(m_viewport.TopLeftY + m_viewport.Height);
}

void GraphicsEngineDx12::SetBackgroundColor(const Ceres::VectorF32& color) noexcept {
	m_backgroundColor = color;
}

void GraphicsEngineDx12::WaitForAsyncTasks() {
	GfxQueInst::GetRef()->WaitForGPU(
		SwapchainInst::GetRef()->GetCurrentBackBufferIndex()
	);
}

void GraphicsEngineDx12::SetShaderPath(const char* path) noexcept {
	m_shaderPath = path;
}

void GraphicsEngineDx12::InitResourceBasedObjects() {
	ModelContainerInst::Init(m_shaderPath.c_str());
}
