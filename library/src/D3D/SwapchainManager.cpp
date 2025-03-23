#include <SwapchainManager.hpp>

// Swapchain Manager
SwapchainManager::SwapchainManager(D3DReusableDescriptorHeap* rtvHeap, UINT bufferCount)
	: m_swapchain{}, m_renderTargetResources{}, m_renderTargets{}, m_presentFlag{ 0u }
{
	m_renderTargetResources.resize(bufferCount);

	for (UINT index = 0u; index < bufferCount; ++index)
	{
		RenderingAttachment& renderTarget = m_renderTargets.emplace_back();

		renderTarget.SetAttachmentHeap(rtvHeap);
	}
}

SwapchainManager::SwapchainManager(
	D3DReusableDescriptorHeap* rtvHeap, UINT bufferCount,
	IDXGIFactory5* factory, const D3DCommandQueue& presentQueue, HWND windowHandle
) : SwapchainManager{ rtvHeap, bufferCount }
{
	Create(factory, presentQueue, windowHandle);
}

void SwapchainManager::Create(
	IDXGIFactory5* factory, const D3DCommandQueue& presentQueue, HWND windowHandle
) {
	DXGI_SWAP_CHAIN_DESC1 desc
	{
		.Width       = 0u,
		.Height      = 0u,
		// This format must be the non-SRGB version here.
		.Format      = DXGI_FORMAT_B8G8R8A8_UNORM,
		.SampleDesc  = DXGI_SAMPLE_DESC{ .Count = 1u, .Quality = 0u },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = static_cast<UINT>(std::size(m_renderTargets)),
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
}

void SwapchainManager::CreateRTVs()
{
	for (size_t index = 0u; index < std::size(m_renderTargets); ++index)
	{
		ComResource renderTargetResource{};

		m_swapchain->GetBuffer(static_cast<UINT>(index), IID_PPV_ARGS(&renderTargetResource));

		m_renderTargets[index].CreateRTV(renderTargetResource.Get(), GetFormat());
		m_renderTargetResources[index] = std::move(renderTargetResource);
	}
}

void SwapchainManager::Resize(UINT width, UINT height)
{
	// Must be called before calling ResizeBuffers.
	for (ComResource& renderTargetResource : m_renderTargetResources)
		renderTargetResource.Reset();

	DXGI_SWAP_CHAIN_DESC1 desc{};
	m_swapchain->GetDesc1(&desc);

	m_swapchain->ResizeBuffers(
		static_cast<UINT>(std::size(m_renderTargets)), width, height, desc.Format,
		desc.Flags
	);

	CreateRTVs();
}

SwapchainManager::Extent SwapchainManager::GetCurrentRenderArea() const noexcept
{
	DXGI_SWAP_CHAIN_DESC1 desc{};
	m_swapchain->GetDesc1(&desc);

	return Extent{ .width = desc.Width, .height = desc.Height };
}
