#include <SwapChainManager.hpp>
#include <GraphicsEngineDx12.hpp>
#include <D3DThrowMacros.hpp>
#include <DeviceManager.hpp>
#include <d3dx12.h>

SwapChainManager::SwapChainManager(
	IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
	std::uint8_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	bool variableRefreshRateAvailable
)
	:
	m_width(width), m_height(height), m_rtvDescSize(0u)
	, m_vsyncFlag(false), m_pRenderTargetViews(bufferCount) {

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.BufferCount = bufferCount;
	desc.Width = m_width;
	desc.Height = m_height;
	desc.Format = GraphicsEngineDx12::RENDER_FORMAT;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1;

	if (variableRefreshRateAvailable)
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGISwapChain1> swapChain;
	HRESULT hr;
	GFX_THROW_FAILED(
		hr, factory->CreateSwapChainForHwnd(
			cmdQueue,
			reinterpret_cast<HWND>(windowHandle),
			&desc,
			nullptr, nullptr,
			&swapChain
		)
	);

	GFX_THROW_FAILED(
		hr, swapChain.As(&m_pSwapChain)
	);

	if (variableRefreshRateAvailable)
		GFX_THROW_FAILED(
			hr, factory->MakeWindowAssociation(
				reinterpret_cast<HWND>(windowHandle),
				DXGI_MWA_NO_ALT_ENTER
			)
		);

	CreateRTVHeap(bufferCount);
	CreateRTVs();
}

void SwapChainManager::CreateRTVs() {
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_pRtvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	HRESULT hr;
	ID3D12Device5* deviceRef = GetD3DDeviceInstance()->GetDeviceRef();

	if (deviceRef) {
		for (std::uint32_t index = 0; index < m_pRenderTargetViews.size(); ++index) {
			GFX_THROW_FAILED(
				hr, m_pSwapChain->GetBuffer(
					index, __uuidof(ID3D12Resource), &m_pRenderTargetViews[index]
				)
			);

			deviceRef->CreateRenderTargetView(
				m_pRenderTargetViews[index].Get(), nullptr, rtvHandle
			);

			rtvHandle.Offset(1, m_rtvDescSize);
		}
	}
}

std::uint32_t SwapChainManager::GetCurrentBackBufferIndex() const noexcept {
	return m_pSwapChain->GetCurrentBackBufferIndex();
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChainManager::ClearRTV(
	ID3D12GraphicsCommandList* commandList, float* clearColor
) noexcept {
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		GetCurrentBackBufferIndex(),
		m_rtvDescSize
	);

	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	return rtvHandle;
}

D3D12_RESOURCE_BARRIER SwapChainManager::GetRenderStateBarrier() const noexcept {
	return CD3DX12_RESOURCE_BARRIER::Transition(
		m_pRenderTargetViews[GetCurrentBackBufferIndex()].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
}

D3D12_RESOURCE_BARRIER SwapChainManager::GetPresentStateBarrier() const noexcept {
	return CD3DX12_RESOURCE_BARRIER::Transition(
		m_pRenderTargetViews[GetCurrentBackBufferIndex()].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
}

void SwapChainManager::ToggleVSync() noexcept {
	m_vsyncFlag = !m_vsyncFlag;
}

void SwapChainManager::PresentWithTear() {
	HRESULT hr;
	GFX_THROW_FAILED(
		hr, m_pSwapChain->Present(
			0, DXGI_PRESENT_ALLOW_TEARING
		)
	);
}

void SwapChainManager::PresentWithoutTear() {
	HRESULT hr;
	GFX_THROW_FAILED(
		hr, m_pSwapChain->Present(
			m_vsyncFlag, 0
		)
	);
}

void SwapChainManager::Resize(std::uint32_t width, std::uint32_t height) {
	if (width != m_width || height != m_height) {
		for (auto& rt : m_pRenderTargetViews)
			rt.Reset();

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		m_pSwapChain->GetDesc1(&desc);

		HRESULT hr;
		GFX_THROW_FAILED(
			hr, m_pSwapChain->ResizeBuffers(
				m_pRenderTargetViews.size(),
				width, height,
				desc.Format,
				desc.Flags
			)
		);

		m_width = width;
		m_height = height;

		CreateRTVs();
	}
}

void SwapChainManager::CreateRTVHeap(std::uint8_t bufferCount) {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = bufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12Device5* deviceRef = GetD3DDeviceInstance()->GetDeviceRef();

	HRESULT hr;
	GFX_THROW_FAILED(
		hr, deviceRef->CreateDescriptorHeap(
			&rtvHeapDesc,
			__uuidof(ID3D12DescriptorHeap),
			&m_pRtvHeap
		)
	);

	m_rtvDescSize = deviceRef->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	);
}

IDXGISwapChain4* SwapChainManager::GetRef() const noexcept {
	return m_pSwapChain.Get();
}
