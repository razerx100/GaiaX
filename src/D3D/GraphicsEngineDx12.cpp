#include <GraphicsEngineDx12.hpp>
#include <IDeviceManager.hpp>
#include <IGraphicsQueueManager.hpp>
#include <ICommandListManager.hpp>
#include <ISwapChainManager.hpp>
#include <D3DThrowMacros.hpp>

GraphicsEngineDx12::GraphicsEngineDx12(
	const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) : m_backgroundColor{0.1f, 0.1f, 0.1f, 0.1f}, m_appName(appName) {
	InitD3DDeviceInstance();

#ifdef _DEBUG
#include <DebugInfoManager.hpp>
	InitDebugInfoManagerInstance();
#endif

	InitGraphicsQueueInstance(
		GetD3DDeviceInstance()->GetDeviceRef(),
		bufferCount
	);
	InitGraphicsListInstance(
		GetD3DDeviceInstance()->GetDeviceRef(),
		bufferCount
	);

	IGraphicsQueueManager* queueRef = GetGraphicsQueueInstance();

	InitSwapChianInstance(
		GetD3DDeviceInstance()->GetFactoryRef(),
		queueRef->GetQueueRef(),
		windowHandle,
		bufferCount, width, height
	);

	queueRef->InitSyncObjects(
		GetD3DDeviceInstance()->GetDeviceRef(),
		GetSwapChainInstance()->GetCurrentBackBufferIndex()
	);

	InitViewPortAndScissor(width, height);

	queueRef->WaitForGPU(
		GetSwapChainInstance()->GetCurrentBackBufferIndex()
	);
}

GraphicsEngineDx12::~GraphicsEngineDx12() noexcept {
	CleanUpSwapChainInstance();
	CleanUpGraphicsListInstance();
	CleanUpGraphicsQueueInstance();
	CleanUpD3DDeviceInstance();
#ifdef _DEBUG
	CleanUpDebugInfoManagerInstance();
#endif
}

void GraphicsEngineDx12::SubmitModels(const IModel* const models, std::uint32_t modelCount) {

}

void GraphicsEngineDx12::Render() {
	IGraphicsQueueManager* queueRef = GetGraphicsQueueInstance();
	ISwapChainManager* swapRef = GetSwapChainInstance();
	ICommandListManager* listManRef = GetGraphicsListInstance();
	ID3D12GraphicsCommandList* commandList = listManRef->GetCommandListRef();

	listManRef->Reset(
		swapRef->GetCurrentBackBufferIndex()
	);
	D3D12_RESOURCE_BARRIER renderBarrier = swapRef->GetRenderStateBarrier();
	commandList->ResourceBarrier(1, &renderBarrier);

	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapRef->ClearRTV(
		commandList, &m_backgroundColor.F32.x
	);

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record objects

	D3D12_RESOURCE_BARRIER presentBarrier = swapRef->GetPresentStateBarrier();
	commandList->ResourceBarrier(1, &presentBarrier);

	listManRef->Close();
	queueRef->ExecuteCommandLists(commandList);

	std::uint32_t backBufferIndex = swapRef->GetCurrentBackBufferIndex();

	swapRef->PresentWithTear();
	queueRef->MoveToNextFrame(backBufferIndex);
}

void GraphicsEngineDx12::Resize(std::uint32_t width, std::uint32_t height) {
	ISwapChainManager* swapRef = GetSwapChainInstance();
	GetGraphicsQueueInstance()->WaitForGPU(
		swapRef->GetCurrentBackBufferIndex()
	);
	swapRef->Resize(width, height);
	InitViewPortAndScissor(width, height);
}

void GraphicsEngineDx12::GetMonitorCoordinates(
	std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
) {
	ComPtr<IDXGIOutput> pOutput;
	HRESULT hr;
	GFX_THROW_FAILED(hr,
		GetSwapChainInstance()->GetRef()->GetContainingOutput(&pOutput)
	);

	DXGI_OUTPUT_DESC desc;
	GFX_THROW_FAILED(hr, pOutput->GetDesc(&desc));

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
	GetGraphicsQueueInstance()->WaitForGPU(
		GetSwapChainInstance()->GetCurrentBackBufferIndex()
	);
}
