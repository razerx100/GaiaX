#include <GraphicsEngineDx12.hpp>
#include <D3DThrowMacros.hpp>
#include <InstanceManager.hpp>

GraphicsEngineDx12::GraphicsEngineDx12(
	const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height,
	size_t bufferCount
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

	ViewPAndScsrInst::Init(width, height);

	CpyQueInst::Init(deviceRef);
	CpyQueInst::GetRef()->InitSyncObjects(deviceRef);

	CpyCmdListInst::Init(deviceRef);
	HeapManagerInst::Init();
	VertexBufferInst::Init();
	IndexBufferInst::Init();
	DescTableManInst::Init();
	TexStorInst::Init();
}

GraphicsEngineDx12::~GraphicsEngineDx12() noexcept {
	TexStorInst::CleanUp();
	DescTableManInst::CleanUp();
	ViewPAndScsrInst::CleanUp();
	CpyCmdListInst::CleanUp();
	CpyQueInst::CleanUp();
	ModelContainerInst::CleanUp();
	VertexBufferInst::CleanUp();
	IndexBufferInst::CleanUp();
	SwapchainInst::CleanUp();
	GfxCmdListInst::CleanUp();
	GfxQueInst::CleanUp();
	DepthBuffInst::CleanUp();
	HeapManagerInst::CleanUp();
	DeviceInst::CleanUp();
#ifdef _DEBUG
	DebugInfoInst::CleanUp();
#endif
}

void GraphicsEngineDx12::SubmitModel(const IModel* const modelRef, bool texture) {
	ModelContainerInst::GetRef()->AddModel(
		DeviceInst::GetRef()->GetDeviceRef(),
		modelRef,
		texture
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
	commandList->ResourceBarrier(1u, &renderBarrier);

	IViewportAndScissorManager* viewportRef = ViewPAndScsrInst::GetRef();

	commandList->RSSetViewports(1u, viewportRef->GetViewportRef());
	commandList->RSSetScissorRects(1u, viewportRef->GetScissorRef());

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapRef->GetRTVHandle();

	swapRef->ClearRTV(
		commandList, &m_backgroundColor.x, rtvHandle
	);

	IDepthBuffer* depthRef = DepthBuffInst::GetRef();

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = depthRef->GetDSVHandle();

	depthRef->ClearDSV(commandList, dsvHandle);

	commandList->OMSetRenderTargets(1u, &rtvHandle, FALSE, &dsvHandle);

	// Record objects
	ModelContainerInst::GetRef()->BindCommands(commandList);

	D3D12_RESOURCE_BARRIER presentBarrier = swapRef->GetPresentStateBarrier();
	commandList->ResourceBarrier(1u, &presentBarrier);

	listManRef->Close();
	queueRef->ExecuteCommandLists(commandList);

	size_t backBufferIndex = swapRef->GetCurrentBackBufferIndex();

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
	ViewPAndScsrInst::GetRef()->Resize(width, height);
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

void GraphicsEngineDx12::SetBackgroundColor(const Ceres::Float32_4& color) noexcept {
	m_backgroundColor = color;
}

void GraphicsEngineDx12::WaitForAsyncTasks() {
	GfxQueInst::GetRef()->WaitForGPU(
		SwapchainInst::GetRef()->GetCurrentBackBufferIndex()
	);
	CpyQueInst::GetRef()->WaitForGPU();
}

void GraphicsEngineDx12::SetShaderPath(const char* path) noexcept {
	m_shaderPath = path;
}

void GraphicsEngineDx12::InitResourceBasedObjects() {
	ModelContainerInst::Init(m_shaderPath.c_str());
}

void GraphicsEngineDx12::ProcessData() {
	ID3D12Device* device = DeviceInst::GetRef()->GetDeviceRef();
	IModelContainer* modelContainerRef = ModelContainerInst::GetRef();
	IDescriptorTableManager* descTableRef = DescTableManInst::GetRef();

	descTableRef->CreateDescriptorTable(device);

	modelContainerRef->CreateBuffers(device);
	modelContainerRef->CopyData();

	TexStorInst::GetRef()->CreateBufferViews(device);

	descTableRef->CopyUploadHeap(device);

	ICommandListManager* copyListManager = CpyCmdListInst::GetRef();
	copyListManager->Reset(0u);
	ID3D12GraphicsCommandList* copyList = copyListManager->GetCommandListRef();

	modelContainerRef->RecordUploadBuffers(copyList);

	copyListManager->Close();

	ICopyQueueManager* copyQue = CpyQueInst::GetRef();
	copyQue->ExecuteCommandLists(copyList);
	copyQue->WaitForGPU();

	descTableRef->ReleaseUploadHeap();
	modelContainerRef->ReleaseUploadBuffers();
}

size_t GraphicsEngineDx12::RegisterResource(
	const void* data, size_t size, bool texture
) {
	return TexStorInst::GetRef()->AddColor(data, size);
}
