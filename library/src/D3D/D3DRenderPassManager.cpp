#include <D3DRenderPassManager.hpp>

void D3DRenderPassManager::AddRenderTarget(
	ID3D12Resource* renderTargetResource, DXGI_FORMAT rtvFormat, bool clearAtStart
) {
	RenderTarget renderTarget{ m_rtvHeap };

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};

	// Only create the render target view, if the resource is valid.
	if (renderTargetResource)
	{
		renderTarget.Create(renderTargetResource, rtvFormat);

		rtvHandle = renderTarget.GetCPUHandle();
	}

	renderTarget.SetClearAtStart(clearAtStart);

	m_rtvHandles.emplace_back(rtvHandle);
	m_renderTargets.emplace_back(std::move(renderTarget));
	m_rtvClearColours.emplace_back(RTVClearColour{ 0.f, 0.f, 0.f, 0.f });
}

void D3DRenderPassManager::RecreateRenderTarget(
	size_t renderTargetIndex, ID3D12Resource* renderTargetResource, DXGI_FORMAT rtvFormat
) {
	RenderTarget& renderTarget = m_renderTargets[renderTargetIndex];

	renderTarget.Create(renderTargetResource, rtvFormat);

	m_rtvHandles[renderTargetIndex] = renderTarget.GetCPUHandle();
}

void D3DRenderPassManager::SetDepthStencilTarget(
	ID3D12Resource* depthStencilTargetResource, DXGI_FORMAT dsvFormat, bool depthClearAtStart,
	bool stencilClearAtStart, D3D12_DSV_FLAGS dsvFlags
) {
	m_depthStencilInfo.dsvFlags |= dsvFlags;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};

	// Only create the depth stencil view if the resource is valid.
	if (depthStencilTargetResource)
	{
		m_depthStencilTarget.Create(
			depthStencilTargetResource, dsvFormat,
			static_cast<D3D12_DSV_FLAGS>(m_depthStencilInfo.dsvFlags)
		);

		dsvHandle = m_depthStencilTarget.GetCPUHandle();
	}

	m_depthStencilTarget.SetClearAtStart(depthClearAtStart || stencilClearAtStart);

	if (depthClearAtStart)
		m_depthStencilInfo.clearFlags |= D3D12_CLEAR_FLAG_DEPTH;

	if (stencilClearAtStart)
		m_depthStencilInfo.clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

	m_dsvHandle = dsvHandle;
}

void D3DRenderPassManager::RecreateDepthStencilTarget(
	ID3D12Resource* depthStencilTargetResource, DXGI_FORMAT dsvFormat
) {
	m_depthStencilTarget.Create(
		depthStencilTargetResource, dsvFormat,
		static_cast<D3D12_DSV_FLAGS>(m_depthStencilInfo.dsvFlags)
	);

	m_dsvHandle = m_depthStencilTarget.GetCPUHandle();
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

bool D3DRenderPassManager::IsDepthClearColourSame(float depthClearColour) const noexcept
{
	return m_depthStencilInfo.depthClearColour == depthClearColour;
}

bool D3DRenderPassManager::IsStencilClearColourSame(std::uint8_t stencilClearColour) const noexcept
{
	return m_depthStencilInfo.stencilClearColour == stencilClearColour;
}

bool D3DRenderPassManager::IsRenderTargetClearColourSame(
	size_t renderTargetIndex, const RTVClearColour& clearValue
) const noexcept {
	const RTVClearColour& rtvClearColour = m_rtvClearColours[renderTargetIndex];

	return rtvClearColour[0] == clearValue[0] &&
		rtvClearColour[1] == clearValue[1] &&
		rtvClearColour[2] == clearValue[2] &&
		rtvClearColour[3] == clearValue[3];
}

void D3DRenderPassManager::StartPass(const D3DCommandList& graphicsCmdList) const noexcept
{
	ID3D12GraphicsCommandList* gfxCmdList = graphicsCmdList.Get();

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

	D3DResourceBarrier<2u>{}
	.AddBarrier(
		ResourceBarrierBuilder{}
		.Transition(
			srcRenderTarget,
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET
		)
	).AddBarrier(
		ResourceBarrierBuilder{}
		.Transition(
			swapchainBackBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT
		)
	).RecordBarriers(gfxCmdList);
}
