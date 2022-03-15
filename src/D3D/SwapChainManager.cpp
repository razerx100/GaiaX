#include <SwapChainManager.hpp>
#include <GraphicsEngineDx12.hpp>
#include <D3DThrowMacros.hpp>
#include <InstanceManager.hpp>
#include <d3dx12.h>

SwapChainManager::SwapChainManager(
	ID3D12Device* device, IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue,
	void* windowHandle,
	size_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	bool variableRefreshRateAvailable
)
	:
	m_width(width), m_height(height), m_rtvDescSize(0u)
	, m_vsyncFlag(false), m_pRenderTargetViews(bufferCount) {

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.BufferCount = static_cast<UINT>(bufferCount);
	desc.Width = m_width;
	desc.Height = m_height;
	desc.Format = GraphicsEngineDx12::RENDER_FORMAT;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1u;

	if (variableRefreshRateAvailable)
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGISwapChain1> swapChain;
	HRESULT hr;
	D3D_THROW_FAILED(
		hr, factory->CreateSwapChainForHwnd(
			cmdQueue,
			reinterpret_cast<HWND>(windowHandle),
			&desc,
			nullptr, nullptr,
			&swapChain
		)
	);

	D3D_THROW_FAILED(
		hr, swapChain.As(&m_pSwapChain)
	);

	if (variableRefreshRateAvailable)
		D3D_THROW_FAILED(
			hr, factory->MakeWindowAssociation(
				reinterpret_cast<HWND>(windowHandle),
				DXGI_MWA_NO_ALT_ENTER
			)
		);

	CreateRTVHeap(device, bufferCount);
	CreateRTVs(device);
}

void SwapChainManager::CreateRTVs(ID3D12Device* device) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_pRtvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	HRESULT hr;
	if (device) {
		for (size_t index = 0u; index < m_pRenderTargetViews.size(); ++index) {
			D3D_THROW_FAILED(
				hr, m_pSwapChain->GetBuffer(
					static_cast<UINT>(index),
					__uuidof(ID3D12Resource), &m_pRenderTargetViews[index]
				)
			);

			device->CreateRenderTargetView(
				m_pRenderTargetViews[index].Get(), nullptr, rtvHandle
			);

			rtvHandle.Offset(1u, static_cast<UINT>(m_rtvDescSize));
		}
	}
}

size_t SwapChainManager::GetCurrentBackBufferIndex() const noexcept {
	return static_cast<size_t>(m_pSwapChain->GetCurrentBackBufferIndex());
}

void SwapChainManager::ClearRTV(
	ID3D12GraphicsCommandList* commandList, float* clearColor,
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle
) noexcept {
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0u, nullptr);
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChainManager::GetRTVHandle() const noexcept {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		static_cast<INT>(GetCurrentBackBufferIndex()),
		static_cast<UINT>(m_rtvDescSize)
	);
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
	D3D_THROW_FAILED(
		hr, m_pSwapChain->Present(
			0u, DXGI_PRESENT_ALLOW_TEARING
		)
	);
}

void SwapChainManager::PresentWithoutTear() {
	HRESULT hr;
	D3D_THROW_FAILED(
		hr, m_pSwapChain->Present(
			m_vsyncFlag, 0u
		)
	);
}

bool SwapChainManager::Resize(
	ID3D12Device* device,
	std::uint32_t width, std::uint32_t height
) {
	if (width != m_width || height != m_height) {
		for (auto& rt : m_pRenderTargetViews)
			rt.Reset();

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		m_pSwapChain->GetDesc1(&desc);

		HRESULT hr;
		D3D_THROW_FAILED(
			hr, m_pSwapChain->ResizeBuffers(
				static_cast<UINT>(m_pRenderTargetViews.size()),
				width, height,
				desc.Format,
				desc.Flags
			)
		);

		m_width = width;
		m_height = height;

		CreateRTVs(device);

		return true;
	}
	else
		return false;
}

void SwapChainManager::CreateRTVHeap(ID3D12Device* device, size_t bufferCount) {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = static_cast<UINT>(bufferCount);
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr;
	D3D_THROW_FAILED(
		hr, device->CreateDescriptorHeap(
			&rtvHeapDesc,
			__uuidof(ID3D12DescriptorHeap),
			&m_pRtvHeap
		)
	);

	m_rtvDescSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	);
}

IDXGISwapChain4* SwapChainManager::GetRef() const noexcept {
	return m_pSwapChain.Get();
}
