#include <SwapChainManager.hpp>
#include <D3DThrowMacros.hpp>
#include <Gaia.hpp>
#include <d3dx12.h>

SwapChainManager::SwapChainManager(const SwapChainCreateInfo& createInfo)
	: m_rtvDescSize(0u), m_vsyncFlag(false), m_pRenderTargetViews(createInfo.bufferCount) {

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.BufferCount = static_cast<UINT>(createInfo.bufferCount);
	desc.Width = createInfo.width;
	desc.Height = createInfo.height;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1u;

	if (createInfo.variableRefreshRate)
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGISwapChain1> swapChain;
	HRESULT hr;
	D3D_THROW_FAILED(
		hr, createInfo.factory->CreateSwapChainForHwnd(
			createInfo.graphicsQueue,
			createInfo.windowHandle,
			&desc,
			nullptr, nullptr,
			&swapChain
		)
	);

	D3D_THROW_FAILED(
		hr, swapChain.As(&m_pSwapChain)
	);

	if (createInfo.variableRefreshRate)
		D3D_THROW_FAILED(
			hr, createInfo.factory->MakeWindowAssociation(
				createInfo.windowHandle,
				DXGI_MWA_NO_ALT_ENTER
			)
		);

	CreateRTVHeap(createInfo.device, createInfo.bufferCount);
	CreateRTVs(createInfo.device);
}

void SwapChainManager::CreateRTVs(ID3D12Device* device) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_pRtvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	HRESULT hr;
	if (device) {
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		for (size_t index = 0u; index < std::size(m_pRenderTargetViews); ++index) {
			D3D_THROW_FAILED(
				hr, m_pSwapChain->GetBuffer(
					static_cast<UINT>(index),
					__uuidof(ID3D12Resource), &m_pRenderTargetViews[index]
				)
			);

			device->CreateRenderTargetView(
				m_pRenderTargetViews[index].Get(), &rtvDesc, rtvHandle
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

D3D12_CPU_DESCRIPTOR_HANDLE SwapChainManager::GetRTVHandle(size_t index) const noexcept {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		static_cast<INT>(index),
		static_cast<UINT>(m_rtvDescSize)
	);
}

D3D12_RESOURCE_BARRIER SwapChainManager::GetRenderStateBarrier(size_t index) const noexcept {
	return CD3DX12_RESOURCE_BARRIER::Transition(
		m_pRenderTargetViews[index].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
}

D3D12_RESOURCE_BARRIER SwapChainManager::GetPresentStateBarrier(size_t index) const noexcept {
	return CD3DX12_RESOURCE_BARRIER::Transition(
		m_pRenderTargetViews[index].Get(),
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

void SwapChainManager::Resize(
	ID3D12Device* device, std::uint32_t width, std::uint32_t height
) {
	for (auto& rt : m_pRenderTargetViews)
		rt.Reset();

	DXGI_SWAP_CHAIN_DESC1 desc = {};
	m_pSwapChain->GetDesc1(&desc);

	HRESULT hr;
	D3D_THROW_FAILED(
		hr, m_pSwapChain->ResizeBuffers(
			static_cast<UINT>(std::size(m_pRenderTargetViews)),
			width, height,
			desc.Format,
			desc.Flags
		)
	);

	CreateRTVs(device);
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
