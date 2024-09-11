#include <SwapChainManager.hpp>
#include <numeric>

// Swapchain Manager
SwapchainManager::SwapchainManager(
	IDXGIFactory5* factory, const D3DCommandQueue& presentQueue, HWND windowHandle, UINT bufferCount
) : SwapchainManager{}
{
	Create(factory, presentQueue, windowHandle, bufferCount);
}

void SwapchainManager::Create(
	IDXGIFactory5* factory, const D3DCommandQueue& presentQueue, HWND windowHandle, UINT bufferCount
) {
	DXGI_SWAP_CHAIN_DESC1 desc
	{
		.Width       = 0u,
		.Height      = 0u,
		.Format      = DXGI_FORMAT_B8G8R8A8_UNORM,
		.SampleDesc  = DXGI_SAMPLE_DESC{ .Count = 1u, .Quality = 0u },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = bufferCount,
		.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD
	};

	BOOL variableRefreshRateSupport = FALSE;

	factory->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING, &variableRefreshRateSupport, static_cast<UINT>(sizeof(BOOL))
	);

	if (variableRefreshRateSupport == TRUE)
	{
		desc.Flags    = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		m_presentFlag = DXGI_PRESENT_ALLOW_TEARING;
	}

	ComPtr<IDXGISwapChain1> swapchain{};
	factory->CreateSwapChainForHwnd(
		presentQueue.GetQueue(), windowHandle, &desc, nullptr, nullptr, &swapchain
	);

	// Must do it after the swapchain creation.
	if (variableRefreshRateSupport == TRUE)
		factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER);

	swapchain->QueryInterface(IID_PPV_ARGS(&m_swapchain));

	m_descriptorIndices.resize(bufferCount, std::numeric_limits<UINT>::max());
	m_renderTargets.resize(bufferCount, nullptr);
}

void SwapchainManager::CreateRTVs(D3DReusableDescriptorHeap& rtvHeap)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc
	{
		.Format        = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
		.Texture2D     = D3D12_TEX2D_RTV
		{
			.MipSlice = 0u
		}
	};

	for (size_t index = 0u; index < std::size(m_renderTargets); ++index)
	{
		m_swapchain->GetBuffer(static_cast<UINT>(index), IID_PPV_ARGS(&m_renderTargets[index]));

		if (m_descriptorIndices[index] != std::numeric_limits<UINT>::max())
			rtvHeap.CreateRTV(m_renderTargets[index].Get(), rtvDesc, m_descriptorIndices[index]);
		else
			m_descriptorIndices[index] = rtvHeap.CreateRTV(m_renderTargets[index].Get(), rtvDesc);
	}
}

void SwapchainManager::Resize(D3DReusableDescriptorHeap& rtvHeap, UINT width, UINT height)
{
	// Must be called before calling ResizeBuffers.
	for (auto& renderTarget : m_renderTargets)
		renderTarget.Reset();

	DXGI_SWAP_CHAIN_DESC1 desc{};
	m_swapchain->GetDesc1(&desc);

	m_swapchain->ResizeBuffers(
		static_cast<UINT>(std::size(m_renderTargets)), width, height, desc.Format,
		desc.Flags
	);

	CreateRTVs(rtvHeap);
}

void SwapchainManager::ClearRTV(
	const D3DCommandList& commandList, const D3DReusableDescriptorHeap& rtvHeap,
	size_t frameIndex, const std::array<float, 4u>& clearColour
) {
	ID3D12GraphicsCommandList* cmdList = commandList.Get();

	cmdList->ClearRenderTargetView(
		rtvHeap.GetCPUHandle(m_descriptorIndices[frameIndex]), std::data(clearColour),
		0u, nullptr
	);
}
