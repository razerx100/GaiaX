#include <D3DExternalRenderPass.hpp>

D3DExternalRenderPass::D3DExternalRenderPass(
	D3DExternalResourceFactory* resourceFactory, D3DReusableDescriptorHeap* rtvHeap,
	D3DReusableDescriptorHeap* dsvHeap
) : m_rtvHeap{ rtvHeap }, m_dsvHeap{ dsvHeap }, m_resourceFactory{ resourceFactory },
	m_renderPassManager{}, m_pipelineDetails{}, m_renderTargetAttachmentDetails{},
	m_depthStencilAttachmentDetails
	{
		.textureIndex = std::numeric_limits<std::uint32_t>::max(),
		.barrierIndex = std::numeric_limits<std::uint32_t>::max()
	},
	m_swapchainCopySource{ std::numeric_limits<std::uint32_t>::max() }, m_firstUseFlags{ 0u },
	m_tempResourceStates(
		s_maxAttachmentCount, ResourceStates
		{
			.beforeState = D3D12_RESOURCE_STATE_COMMON,
			.afterState  = D3D12_RESOURCE_STATE_COMMON
		}
	)
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
	if (m_depthStencilAttachmentDetails.textureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			m_depthStencilAttachmentDetails.textureIndex
		);

		if (!std::empty(m_tempResourceStates))
		{
			// If this is the first use of the depth texture, set the before state to the
			// very last state, as the resource will be created in that state and on the future
			// frames, this will be the before state.
			if (m_firstUseFlags[s_depthAttachmentIndex])
				m_tempResourceStates[s_depthAttachmentIndex].beforeState
					= externalTexture->GetCurrentState();

			const ResourceStates resourceStates = m_tempResourceStates[s_depthAttachmentIndex];

			m_depthStencilAttachmentDetails.barrierIndex = m_renderPassManager.AddStartBarrier(
				ResourceBarrierBuilder{}.Transition(
					externalTexture->GetTexture().Get(),
					resourceStates.beforeState, resourceStates.afterState
				)
			);
		}

		m_renderPassManager.SetDepthStencil(
			externalTexture->GetAttachmentHandle(), m_depthStencilAttachmentDetails.barrierIndex,
			externalTexture->GetTexture().Get()
		);
	}

	const size_t renderTargetCount = std::size(m_renderTargetAttachmentDetails);

	for (size_t index = 0u; index < renderTargetCount; ++index)
	{
		AttachmentDetails& renderTargetDetails = m_renderTargetAttachmentDetails[index];

		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			renderTargetDetails.textureIndex
		);

		if (!std::empty(m_tempResourceStates))
		{
			// If this is the first use of the render texture, set the before state to the
			// very last state, as the resource will be created in that state and on the future
			// frames, this will be the before state.
			if (m_firstUseFlags[index])
				m_tempResourceStates[index].beforeState = externalTexture->GetCurrentState();

			const ResourceStates resourceStates = m_tempResourceStates[index];

			renderTargetDetails.barrierIndex = m_renderPassManager.AddStartBarrier(
				ResourceBarrierBuilder{}.Transition(
					externalTexture->GetTexture().Get(),
					resourceStates.beforeState, resourceStates.afterState
				)
			);
		}

		m_renderPassManager.SetRenderTarget(
			index, externalTexture->GetAttachmentHandle(), renderTargetDetails.barrierIndex,
			externalTexture->GetTexture().Get()
		);
	}

	if (!std::empty(m_tempResourceStates))
		m_tempResourceStates = std::vector<ResourceStates>{};
}

void D3DExternalRenderPass::SetDepthStencil(
	std::uint32_t externalTextureIndex, D3D12_RESOURCE_STATES newState, D3D12_DSV_FLAGS dsvFlag,
	bool clearDepth, bool clearStencil
) {
	if (m_depthStencilAttachmentDetails.textureIndex == std::numeric_limits<std::uint32_t>::max())
	{
		m_depthStencilAttachmentDetails.textureIndex = externalTextureIndex;

		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			externalTextureIndex
		);

		externalTexture->SetAttachmentHeap(m_dsvHeap);

		// Only set the default colours if the state is common. Otherwise the
		// clear colour will cause issues if we use the same texture in multiple passes.
		// Can't use the same texture in multiple passes with multiple different clear values without
		// performance hit. But it should be fine to clear on one, but if we set the default
		// state on every pass, the default value will overwrite any other values any other
		// passes had set. But we have to set the default colours, since the colour variable is a union
		// and we won't know if it is gonna be an RTV or DSV at the start.
		m_firstUseFlags[s_depthAttachmentIndex]
			= externalTexture->GetCurrentState() == D3D12_RESOURCE_STATE_COMMON;

		if (m_firstUseFlags[s_depthAttachmentIndex])
			externalTexture->SetDepthStencilClearColour(
				D3D12_DEPTH_STENCIL_VALUE{ .Depth = 1.f, .Stencil = 0u }
			);
	}

	D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
		m_depthStencilAttachmentDetails.textureIndex
	);

	const D3D12_RESOURCE_STATES oldState = externalTexture->GetCurrentState();

	externalTexture->SetCurrentState(newState);

	if (!std::empty(m_tempResourceStates))
		m_tempResourceStates[s_depthAttachmentIndex]
			= ResourceStates{ .beforeState = oldState, .afterState  = newState };

	// The DSV flag is to make either Depth or Stencil read only. Can't set any of that
	// if any of the passes need depth write.
	if (dsvFlag == D3D12_DSV_FLAG_NONE)
		externalTexture->SetDSVFlag(dsvFlag);
	else if (m_firstUseFlags[s_depthAttachmentIndex] || externalTexture->GetDSVFlags())
		externalTexture->AddDSVFlag(dsvFlag);

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
	if (m_depthStencilAttachmentDetails.textureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			m_depthStencilAttachmentDetails.textureIndex
		);

		externalTexture->SetDepthClearColour(clearColour);

		// Only recreate if there is an already existing texture.
		if (externalTexture->IsTextureCreated())
			externalTexture->Recreate(ExternalTexture2DType::Depth);
	}

	m_renderPassManager.SetDepthClearValue(clearColour);
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

	if (m_depthStencilAttachmentDetails.textureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
			m_depthStencilAttachmentDetails.textureIndex
		);

		externalTexture->SetStencilClearColour(u8ClearColour);

		// Only recreate if there is an already existing texture.
		if (externalTexture->IsTextureCreated())
			externalTexture->Recreate(ExternalTexture2DType::Stencil);
	}

	m_renderPassManager.SetStencilClearValue(u8ClearColour);
}

std::uint32_t D3DExternalRenderPass::AddRenderTarget(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	[[maybe_unused]] ExternalAttachmentStoreOp storeOp
) {
	const bool clearRenderTarget = loadOp == ExternalAttachmentLoadOp::Clear;

	D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(externalTextureIndex);

	externalTexture->SetAttachmentHeap(m_rtvHeap);

	// Only set the default colours if the state is common. Otherwise the
	// clear colour will cause issues if we use the same texture in multiple passes.
	// Can't use the same texture in multiple passes with multiple different clear values without
	// performance hit. But it should be fine to clear on one, but if we set the default
	// state on every pass, the default value will overwrite any other values any other
	// passes had set. But we have to set the default colours, since the colour variable is a union
	// and we won't know if it is gonna be an RTV or DSV at the start.
	const bool firstUse = externalTexture->GetCurrentState() == D3D12_RESOURCE_STATE_COMMON;

	if (firstUse)
		externalTexture->SetRenderTargetClearColour({ 0.f, 0.f, 0.f, 0.f });

	const size_t renderTargetLocalIndex = std::size(m_renderTargetAttachmentDetails);

	m_firstUseFlags[renderTargetLocalIndex] = firstUse;

	const D3D12_RESOURCE_STATES beforeState = externalTexture->GetCurrentState();
	const D3D12_RESOURCE_STATES afterState  = D3D12_RESOURCE_STATE_RENDER_TARGET;

	externalTexture->SetCurrentState(afterState);

	if (!std::empty(m_tempResourceStates))
		m_tempResourceStates[renderTargetLocalIndex]
			= ResourceStates{ .beforeState = beforeState, .afterState  = afterState };

	const auto u32RenderTargetLocalIndex = static_cast<std::uint32_t>(renderTargetLocalIndex);

	m_renderTargetAttachmentDetails.emplace_back(
		AttachmentDetails{ .textureIndex = externalTextureIndex }
	);

	m_renderPassManager.AddRenderTarget(clearRenderTarget);

	return u32RenderTargetLocalIndex;
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

	const std::uint32_t renderTargetTextureIndex
		= m_renderTargetAttachmentDetails[zRenderTargetIndex].textureIndex;

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

void D3DExternalRenderPass::SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept
{
	m_swapchainCopySource = m_renderTargetAttachmentDetails[renderTargetIndex].textureIndex;

	D3DExternalTexture* externalTexture = m_resourceFactory->GetD3DExternalTexture(
		m_swapchainCopySource
	);

	externalTexture->SetCurrentState(D3D12_RESOURCE_STATE_COPY_SOURCE);
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
