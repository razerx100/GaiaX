#include <SwapChainManager.hpp>
#include <Gaia.hpp>
#include <d3dx12.h>

SwapChainManager::SwapChainManager(const Args& arguments)
	: m_rtvDescSize{ 0u }, m_vsyncFlag{ false },
	m_pRenderTargetViews{ arguments.bufferCount } {

	ID3D12Device* device = arguments.device;
	UINT bufferCount = static_cast<UINT>(arguments.bufferCount);

	DXGI_SWAP_CHAIN_DESC1 desc{
		.Width = arguments.width,
		.Height = arguments.height,
		.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = bufferCount,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
	};
	desc.SampleDesc.Count = 1u;

	bool variableRefreshRate = arguments.variableRefreshRate;
	if (variableRefreshRate)
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	IDXGIFactory2* factory = arguments.factory;
	HWND windowHandle = arguments.windowHandle;

	ComPtr<IDXGISwapChain1> swapChain;
	factory->CreateSwapChainForHwnd(
		arguments.graphicsQueue, windowHandle, &desc, nullptr, nullptr, &swapChain
	);
	swapChain.As(&m_pSwapChain);

	if (variableRefreshRate)
		factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER);

	CreateRTVHeap(device, bufferCount);
	CreateRTVs(device);
}

void SwapChainManager::CreateRTVs(ID3D12Device* device) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_pRtvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	if (device) {
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		for (size_t index = 0u; index < std::size(m_pRenderTargetViews); ++index) {
			m_pSwapChain->GetBuffer(
				static_cast<UINT>(index), IID_PPV_ARGS( & m_pRenderTargetViews[index])
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
		static_cast<INT>(index), static_cast<UINT>(m_rtvDescSize)
	);
}

ID3D12Resource* SwapChainManager::GetRTV(size_t index) const noexcept {
	return m_pRenderTargetViews[index].Get();
}

void SwapChainManager::ToggleVSync() noexcept {
	m_vsyncFlag = !m_vsyncFlag;
}

void SwapChainManager::PresentWithTear() {
	m_pSwapChain->Present(0u, DXGI_PRESENT_ALLOW_TEARING);
}

void SwapChainManager::PresentWithoutTear() {
	m_pSwapChain->Present(m_vsyncFlag, 0u);
}

void SwapChainManager::Resize(
	ID3D12Device* device, std::uint32_t width, std::uint32_t height
) {
	for (auto& rt : m_pRenderTargetViews)
		rt.Reset();

	DXGI_SWAP_CHAIN_DESC1 desc{};
	m_pSwapChain->GetDesc1(&desc);

	m_pSwapChain->ResizeBuffers(
		static_cast<UINT>(std::size(m_pRenderTargetViews)), width, height, desc.Format,
		desc.Flags
	);

	CreateRTVs(device);
}

void SwapChainManager::CreateRTVHeap(ID3D12Device* device, UINT bufferCount) {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = bufferCount,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};

	device->CreateDescriptorHeap(&rtvHeapDesc,IID_PPV_ARGS(&m_pRtvHeap));

	m_rtvDescSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	);
}

IDXGISwapChain4* SwapChainManager::GetRef() const noexcept {
	return m_pSwapChain.Get();
}
