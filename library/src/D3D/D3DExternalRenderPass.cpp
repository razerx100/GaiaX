#include <D3DExternalRenderPass.hpp>

D3DExternalRenderPass::D3DExternalRenderPass(
	D3DExternalResourceFactory* resourceFactory, D3DReusableDescriptorHeap* rtvHeap,
	D3DReusableDescriptorHeap* dsvHeap
) : m_resourceFactory{ resourceFactory }, m_renderPassManager{ rtvHeap, dsvHeap },
	m_pipelineDetails{}, m_renderTargetDetails{},
	m_depthStencilDetails
	{
		.textureIndex = std::numeric_limits<std::uint32_t>::max(),
		.barrierIndex = std::numeric_limits<std::uint32_t>::max()
	}, m_swapchainCopySource{ std::numeric_limits<std::uint32_t>::max() }
{}

void D3DExternalRenderPass::AddPipeline(std::uint32_t pipelineIndex, bool sorted)
{
	m_pipelineDetails.emplace_back(
		PipelineDetails
		{
			.pipelineGlobalIndex = pipelineIndex,
			.renderSorted        = sorted
		}
	);
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
	if (m_depthStencilDetails.textureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			m_depthStencilDetails.textureIndex
		);

		const Texture& depthStencilTexture = externalTexture->GetTexture();

		m_renderPassManager.RecreateDepthStencilTarget(
			m_depthStencilDetails.barrierIndex, depthStencilTexture.Get(),
			depthStencilTexture.Format()
		);
	}

	const size_t renderTargetCount = std::size(m_renderTargetDetails);

	for (size_t index = 0u; index < renderTargetCount; ++index)
	{
		const AttachmentDetails& renderTargetDetails = m_renderTargetDetails[index];

		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			renderTargetDetails.textureIndex
		);

		const Texture& renderTargetTexture = externalTexture->GetTexture();

		m_renderPassManager.RecreateRenderTarget(
			index, renderTargetDetails.barrierIndex, renderTargetTexture.Get(),
			renderTargetTexture.Format()
		);
	}
}

void D3DExternalRenderPass::SetDepthStencil(
	std::uint32_t externalTextureIndex, D3D12_RESOURCE_STATES newState, D3D12_DSV_FLAGS dsvFlags,
	bool clearDepth, bool clearStencil
) {
	if (m_depthStencilDetails.textureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		m_depthStencilDetails.textureIndex  = externalTextureIndex;

		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			externalTextureIndex
		);

		m_depthStencilDetails.barrierIndex = m_renderPassManager.AddStartBarrier(
			externalTexture->TransitionState(newState)
		);
	}

	D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
		m_depthStencilDetails.textureIndex
	);

	// If the current state is either common / depth read, we must replace it with depth write.
	if (externalTexture->GetCurrentState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
	{
		m_renderPassManager.SetTransitionAfterState(
			m_depthStencilDetails.barrierIndex, newState
		);

		externalTexture->SetCurrentState(newState);
	}

	const Texture& depthTexture = externalTexture->GetTexture();

	m_renderPassManager.SetDepthStencilTarget(
		depthTexture.Get(), depthTexture.Format(), clearDepth, clearStencil, dsvFlags
	);
}

void D3DExternalRenderPass::SetDepthTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	ExternalAttachmentStoreOp storeOp
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

void D3DExternalRenderPass::SetDepthClearColour(float clearColour) noexcept
{
	m_renderPassManager.SetDepthClearValue(clearColour);
}

void D3DExternalRenderPass::SetStencilTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	ExternalAttachmentStoreOp storeOp
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

void D3DExternalRenderPass::SetStencilClearColour(std::uint32_t clearColour) noexcept
{
	m_renderPassManager.SetStencilClearValue(static_cast<std::uint8_t>(clearColour));
}

std::uint32_t D3DExternalRenderPass::AddRenderTarget(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	ExternalAttachmentStoreOp storeOp
) {
	const bool clearRenderTarget
		= loadOp == ExternalAttachmentLoadOp::Clear || storeOp == ExternalAttachmentStoreOp::Store;

	D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(externalTextureIndex);

	const std::uint32_t renderTargetBarrierIndex = m_renderPassManager.AddStartBarrier(
		externalTexture->TransitionState(D3D12_RESOURCE_STATE_RENDER_TARGET)
	);

	const auto renderTargetIndex = static_cast<std::uint32_t>(std::size(m_renderTargetDetails));

	m_renderTargetDetails.emplace_back(
		AttachmentDetails
		{
			.textureIndex = externalTextureIndex,
			.barrierIndex = renderTargetBarrierIndex
		}
	);

	const Texture& renderTexture = externalTexture->GetTexture();

	m_renderPassManager.AddRenderTarget(renderTexture.Get(), renderTexture.Format(), clearRenderTarget);

	return renderTargetIndex;
}

void D3DExternalRenderPass::SetRenderTargetClearColour(
	std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
) noexcept {
	const std::array<float, 4u> d3dClearColour
	{
		clearColour.x,
		clearColour.y,
		clearColour.z,
		clearColour.w
	};

	m_renderPassManager.SetRenderTargetClearValue(renderTargetIndex, d3dClearColour);
}

void D3DExternalRenderPass::SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept
{
	m_swapchainCopySource = m_renderTargetDetails[renderTargetIndex].textureIndex;
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
