#include <D3DExternalRenderPass.hpp>

D3DExternalRenderPass::D3DExternalRenderPass(
	D3DExternalResourceFactory* resourceFactory, D3DReusableDescriptorHeap* rtvHeap,
	D3DReusableDescriptorHeap* dsvHeap
) : m_rtvHeap{ rtvHeap }, m_dsvHeap{ dsvHeap }, m_resourceFactory{ resourceFactory },
	m_renderPassManager{}, m_pipelineDetails{}, m_renderTargetTextureIndices{},
	m_depthStencilTextureIndex{ std::numeric_limits<std::uint32_t>::max() },
	m_swapchainCopySource{ std::numeric_limits<std::uint32_t>::max() }
{}

void D3DExternalRenderPass::AddPipeline(std::uint32_t pipelineIndex)
{
	m_pipelineDetails.emplace_back(PipelineDetails{ .pipelineGlobalIndex = pipelineIndex });
}

void D3DExternalRenderPass::RemoveModelBundle(std::uint32_t bundleIndex) noexcept
{
	for (size_t index = 0u; index < std::size(m_pipelineDetails); ++index)
	{
		PipelineDetails& pipelineDetails = m_pipelineDetails[index];

		std::vector<std::uint32_t>& bundleIndices        = pipelineDetails.modelBundleIndices;
		std::vector<std::uint32_t>& pipelineLocalIndices = pipelineDetails.pipelineLocalIndices;

		auto result = std::ranges::find(bundleIndices, bundleIndex);

		if (result != std::end(bundleIndices))
		{
			pipelineLocalIndices.erase(
				std::next(
					std::begin(pipelineLocalIndices), std::distance(std::begin(bundleIndices), result)
				)
			);

			bundleIndices.erase(result);
		}
	}
}

void D3DExternalRenderPass::RemovePipeline(std::uint32_t pipelineIndex) noexcept
{
	auto result = std::ranges::find(
		m_pipelineDetails, pipelineIndex,
		[](const PipelineDetails& indexDetails)
		{
			return indexDetails.pipelineGlobalIndex;
		}
	);

	if (result != std::end(m_pipelineDetails))
		m_pipelineDetails.erase(result);
}

void D3DExternalRenderPass::ResetAttachmentReferences()
{
	if (m_depthStencilTextureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			m_depthStencilTextureIndex
		);

		m_renderPassManager.SetDSVHandle(externalTexture->GetAttachmentHandle());
	}

	const size_t renderTargetCount = std::size(m_renderTargetTextureIndices);

	for (size_t index = 0u; index < renderTargetCount; ++index)
	{
		std::uint32_t renderTargetTextureIndex = m_renderTargetTextureIndices[index];

		D3DExternalTexture* externalTexture    = m_resourceFactory->GetD3DExternalTexture(
			renderTargetTextureIndex
		);

		m_renderPassManager.SetRTVHandle(index, externalTexture->GetAttachmentHandle());
	}
}

void D3DExternalRenderPass::SetDepthStencil(
	std::uint32_t externalTextureIndex, D3D12_RESOURCE_STATES newState, D3D12_DSV_FLAGS dsvFlag,
	bool clearDepth, bool clearStencil
) {
	if (m_depthStencilTextureIndex == std::numeric_limits<std::uint32_t>::max())
	{
		m_depthStencilTextureIndex          = externalTextureIndex;

		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			externalTextureIndex
		);

		externalTexture->SetAttachmentHeap(m_dsvHeap);

		// Set the default clear colours. Even if they aren't used.
		externalTexture->SetDepthStencilClearColour(
			D3D12_DEPTH_STENCIL_VALUE{ .Depth = 1.f, .Stencil = 0u }
		);
	}

	D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
		m_depthStencilTextureIndex
	);

	externalTexture->AddDSVFlag(dsvFlag);

	// We don't need to change the state if the current state is already DEPTH WRITE.
	if (externalTexture->GetCurrentState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
		externalTexture->SetCurrentState(newState);

	m_renderPassManager.SetDepthStencilTarget(clearDepth, clearStencil);
}

void D3DExternalRenderPass::SetDepthTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp, ExternalAttachmentStoreOp storeOp
) {
	D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_DEPTH_READ;
	D3D12_DSV_FLAGS dsvFlags       = D3D12_DSV_FLAG_NONE;

	const bool clearDepth          = loadOp == ExternalAttachmentLoadOp::Clear ;

	if (clearDepth || storeOp == ExternalAttachmentStoreOp::Store)
		newState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	else
		dsvFlags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;

	SetDepthStencil(externalTextureIndex, newState, dsvFlags, clearDepth, false);
}

void D3DExternalRenderPass::SetDepthClearColour(float clearColour)
{
	if (!m_renderPassManager.IsDepthClearColourSame(clearColour))
	{
		if (m_depthStencilTextureIndex != std::numeric_limits<std::uint32_t>::max())
		{
			D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
				m_depthStencilTextureIndex
			);

			externalTexture->SetDepthClearColour(clearColour);

			// Only recreate if there is an already existing texture.
			if (externalTexture->IsTextureCreated())
				externalTexture->Recreate(ExternalTexture2DType::Depth);
		}

		m_renderPassManager.SetDepthClearValue(clearColour);
	}
}

void D3DExternalRenderPass::SetStencilTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp, ExternalAttachmentStoreOp storeOp
) {
	D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_DEPTH_READ;
	D3D12_DSV_FLAGS dsvFlags       = D3D12_DSV_FLAG_NONE;

	const bool clearStencil        = loadOp == ExternalAttachmentLoadOp::Clear ;

	if (clearStencil || storeOp == ExternalAttachmentStoreOp::Store)
		newState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	else
		dsvFlags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;

	SetDepthStencil(externalTextureIndex, newState, dsvFlags, false, clearStencil);
}

void D3DExternalRenderPass::SetStencilClearColour(std::uint32_t clearColour)
{
	const auto u8ClearColour = static_cast<UINT8>(clearColour);

	if (!m_renderPassManager.IsStencilClearColourSame(u8ClearColour))
	{
		if (m_depthStencilTextureIndex != std::numeric_limits<std::uint32_t>::max())
		{
			D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
				m_depthStencilTextureIndex
			);

			externalTexture->SetStencilClearColour(u8ClearColour);

			// Only recreate if there is an already existing texture.
			if (externalTexture->IsTextureCreated())
				externalTexture->Recreate(ExternalTexture2DType::Stencil);
		}

		m_renderPassManager.SetStencilClearValue(u8ClearColour);
	}
}

std::uint32_t D3DExternalRenderPass::AddRenderTarget(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	[[maybe_unused]] ExternalAttachmentStoreOp storeOp
) {
	const bool clearRenderTarget = loadOp == ExternalAttachmentLoadOp::Clear;

	D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(externalTextureIndex);

	externalTexture->SetAttachmentHeap(m_rtvHeap);

	// Set the default clear colours. Even if they aren't used.
	externalTexture->SetRenderTargetClearColour({ 0.f, 0.f, 0.f, 0.f });

	externalTexture->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

	const auto renderTargetLocalIndex
		= static_cast<std::uint32_t>(std::size(m_renderTargetTextureIndices));

	m_renderTargetTextureIndices.emplace_back(externalTextureIndex);

	m_renderPassManager.AddRenderTarget(clearRenderTarget);

	return renderTargetLocalIndex;
}

void D3DExternalRenderPass::SetRenderTargetClearColour(
	std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
) {
	const std::array<float, 4u> d3dClearColour
	{
		clearColour.x,
		clearColour.y,
		clearColour.z,
		clearColour.w
	};

	const auto zRenderTargetIndex = static_cast<size_t>(renderTargetIndex);

	if (!m_renderPassManager.IsRenderTargetClearColourSame(zRenderTargetIndex, d3dClearColour))
	{
		const std::uint32_t renderTargetTextureIndex = m_renderTargetTextureIndices[zRenderTargetIndex];

		if (renderTargetTextureIndex != std::numeric_limits<std::uint32_t>::max())
		{
			D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
				renderTargetTextureIndex
			);

			externalTexture->SetRenderTargetClearColour(d3dClearColour);

			// Only recreate if there is an already existing texture.
			if (externalTexture->IsTextureCreated())
				externalTexture->Recreate(ExternalTexture2DType::RenderTarget);
		}

		m_renderPassManager.SetRenderTargetClearValue(zRenderTargetIndex, d3dClearColour);
	}
}

void D3DExternalRenderPass::SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept
{
	m_swapchainCopySource = m_renderTargetTextureIndices[renderTargetIndex];
}

void D3DExternalRenderPass::StartPass(const D3DCommandList& graphicsCmdList) const noexcept
{
	m_renderPassManager.StartPass(graphicsCmdList);
}

void D3DExternalRenderPass::EndPassForSwapchain(
	const D3DCommandList& graphicsCmdList, ID3D12Resource* swapchainBackBuffer
) const noexcept {
	D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(m_swapchainCopySource);

	m_renderPassManager.EndPassForSwapchain(
		graphicsCmdList, externalTexture->GetTexture().Get(), swapchainBackBuffer
	);
}
