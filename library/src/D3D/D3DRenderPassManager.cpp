#include <D3DRenderPassManager.hpp>

void D3DRenderPassManager::AddRenderTarget(
	ID3D12Resource* renderTargetResource, DXGI_FORMAT rtvFormat, bool clearAtStart
) {
	RenderTarget renderTarget{ m_rtvHeap };

	renderTarget.Create(renderTargetResource, rtvFormat);
	renderTarget.SetClearAtStart(clearAtStart);

	m_rtvHandles.emplace_back(renderTarget.GetCPUHandle());
	m_renderTargets.emplace_back(std::move(renderTarget));
	m_rtvClearColours.emplace_back(RTVClearColour{ 0.f, 0.f, 0.f, 0.f });
}

std::uint32_t D3DRenderPassManager::AddStartBarrier(const ResourceBarrierBuilder& barrierBuilder) noexcept
{
	const std::uint32_t barrierIndex = m_startImageBarriers.GetCount();

	m_startImageBarriers.AddBarrier(barrierBuilder);

	return barrierIndex;
}

void D3DRenderPassManager::SetTransitionAfterState(
	size_t barrierIndex, D3D12_RESOURCE_STATES afterState
) noexcept {
	m_startImageBarriers.SetTransitionAfterState(barrierIndex, afterState);
}

void D3DRenderPassManager::RecreateRenderTarget(
	size_t renderTargetIndex, size_t barrierIndex, ID3D12Resource* renderTargetResource,
	DXGI_FORMAT rtvFormat
) {
	RenderTarget& renderTarget = m_renderTargets[renderTargetIndex];

	renderTarget.Create(renderTargetResource, rtvFormat);

	m_startImageBarriers.SetTransitionResource(barrierIndex, renderTargetResource);

	// No need to fetch the cpu handle again.
}

void D3DRenderPassManager::SetDepthStencilTarget(
	ID3D12Resource* depthStencilTargetResource, DXGI_FORMAT dsvFormat, bool depthClearAtStart,
	bool stencilClearAtStart, D3D12_DSV_FLAGS dsvFlags
) {
	m_depthStencilInfo.dsvFlags |= dsvFlags;

	m_depthStencilTarget.Create(
		depthStencilTargetResource, dsvFormat,
		static_cast<D3D12_DSV_FLAGS>(m_depthStencilInfo.dsvFlags)
	);
	m_depthStencilTarget.SetClearAtStart(depthClearAtStart || stencilClearAtStart);

	if (depthClearAtStart)
		m_depthStencilInfo.clearFlags |= D3D12_CLEAR_FLAG_DEPTH;

	if (stencilClearAtStart)
		m_depthStencilInfo.clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

	m_dsvHandle = m_depthStencilTarget.GetCPUHandle();
}

void D3DRenderPassManager::RecreateDepthStencilTarget(
	size_t barrierIndex, ID3D12Resource* depthStencilTargetResource, DXGI_FORMAT dsvFormat
) {
	m_depthStencilTarget.Create(
		depthStencilTargetResource, dsvFormat,
		static_cast<D3D12_DSV_FLAGS>(m_depthStencilInfo.dsvFlags)
	);

	m_startImageBarriers.SetTransitionResource(barrierIndex, depthStencilTargetResource);
}

void D3DRenderPassManager::SetDepthClearValue(float depthClearValue) noexcept
{
	m_depthStencilInfo.depthClearColour = depthClearValue;
}

void D3DRenderPassManager::SetStencilClearValue(std::uint8_t stencilClearValue) noexcept
{
	m_depthStencilInfo.stencilClearColour = stencilClearValue;
}

void D3DRenderPassManager::SetRenderTargetClearValue(
	size_t renderTargetIndex, const RTVClearColour& clearValue
) noexcept {
	m_rtvClearColours[renderTargetIndex] = clearValue;
}

void D3DRenderPassManager::StartPass(const D3DCommandList& graphicsCmdList) const noexcept
{
	ID3D12GraphicsCommandList* gfxCmdList = graphicsCmdList.Get();

	m_startImageBarriers.RecordBarriers(gfxCmdList);

	D3D12_CPU_DESCRIPTOR_HANDLE const* dsvHandle = nullptr;

	if (m_depthStencilTarget.ShouldClearAtStart())
	{
		dsvHandle = &m_dsvHandle;

		gfxCmdList->ClearDepthStencilView(
			m_dsvHandle,
			static_cast<D3D12_CLEAR_FLAGS>(m_depthStencilInfo.clearFlags),
			m_depthStencilInfo.depthClearColour, m_depthStencilInfo.stencilClearColour,
			0u, nullptr
		);
	}

	const size_t renderTargetCount = std::size(m_renderTargets);

	for (size_t index = 0u; index < renderTargetCount; ++index)
		if (m_renderTargets[index].ShouldClearAtStart())
			gfxCmdList->ClearRenderTargetView(
				m_rtvHandles[index], std::data(m_rtvClearColours[index]), 0u, nullptr
			);

	gfxCmdList->OMSetRenderTargets(
		static_cast<UINT>(renderTargetCount), std::data(m_rtvHandles),
		FALSE, dsvHandle
	);
}

void D3DRenderPassManager::EndPassForSwapchain(
	const D3DCommandList& graphicsCmdList, ID3D12Resource* srcRenderTarget,
	ID3D12Resource* swapchainBackBuffer
) const noexcept {
	ID3D12GraphicsCommandList* gfxCmdList = graphicsCmdList.Get();

	D3DResourceBarrier<2u>{}
	.AddBarrier(
		ResourceBarrierBuilder{}
		.Transition(
			srcRenderTarget,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE
		)
	).AddBarrier(
		ResourceBarrierBuilder{}
		.Transition(
			swapchainBackBuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST
		)
	).RecordBarriers(gfxCmdList);

	graphicsCmdList.CopyTexture(srcRenderTarget, swapchainBackBuffer, 0u, 0u);

	D3DResourceBarrier<>{}
	.AddBarrier(
		ResourceBarrierBuilder{}
		.Transition(
			swapchainBackBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT
		)
	).RecordBarriers(gfxCmdList);
}
