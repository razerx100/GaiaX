#include <D3DRenderPassManager.hpp>

namespace Gaia
{
void D3DRenderPassManager::AddRenderTarget(bool clearAtStart)
{
	// The RTV handle won't be a valid one before the resource is created. So, add an invalid one
	// to reserve the space.
	m_rtvHandles.emplace_back(D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0u });
	m_rtvClearFlags.emplace_back(clearAtStart);
	m_rtvClearColours.emplace_back(RTVClearColour{ 0.f, 0.f, 0.f, 0.f });
}

std::uint32_t D3DRenderPassManager::AddStartBarrier(const ResourceBarrierBuilder& barrierBuilder) noexcept
{
	auto barrierIndex = std::numeric_limits<std::uint32_t>::max();

	const D3D12_RESOURCE_TRANSITION_BARRIER& transtion = barrierBuilder.GetTransition();

	if (transtion.StateBefore != transtion.StateAfter)
	{
		barrierIndex = m_startBarriers.GetCount();

		m_startBarriers.AddBarrier(barrierBuilder);
	}

	return barrierIndex;
}

void D3DRenderPassManager::SetTransitionBarrierResource(
	std::uint32_t barrierIndex, ID3D12Resource* resource
) noexcept {
	if (barrierIndex != std::numeric_limits<std::uint32_t>::max())
		m_startBarriers.SetTransitionResource(barrierIndex, resource);
}

void D3DRenderPassManager::SetRenderTarget(
	size_t renderTargetIndex, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
	std::uint32_t barrierIndex, ID3D12Resource* renderTarget
) noexcept {
	m_rtvHandles[renderTargetIndex] = rtvHandle;

	SetTransitionBarrierResource(barrierIndex, renderTarget);
}

void D3DRenderPassManager::SetDepthStencilTarget(bool depthClearAtStart, bool stencilClearAtStart) noexcept
{
	if (depthClearAtStart)
		m_depthStencilInfo.clearFlags |= D3D12_CLEAR_FLAG_DEPTH;

	if (stencilClearAtStart)
		m_depthStencilInfo.clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
}

void D3DRenderPassManager::SetDepthStencil(
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, std::uint32_t barrierIndex, ID3D12Resource* depthStencilTarget
) noexcept {
	m_dsvHandle = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE>(dsvHandle);

	SetTransitionBarrierResource(barrierIndex, depthStencilTarget);
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

	if (m_startBarriers.GetCount())
		m_startBarriers.RecordBarriers(gfxCmdList);

	if (m_depthStencilInfo.clearFlags)
		gfxCmdList->ClearDepthStencilView(
			*m_dsvHandle,
			static_cast<D3D12_CLEAR_FLAGS>(m_depthStencilInfo.clearFlags),
			m_depthStencilInfo.depthClearColour, m_depthStencilInfo.stencilClearColour,
			0u, nullptr
		);

	const size_t renderTargetCount = std::size(m_rtvClearFlags);

	for (size_t index = 0u; index < renderTargetCount; ++index)
		if (m_rtvClearFlags[index])
			gfxCmdList->ClearRenderTargetView(
				m_rtvHandles[index], std::data(m_rtvClearColours[index]), 0u, nullptr
			);

	gfxCmdList->OMSetRenderTargets(
		static_cast<UINT>(renderTargetCount), std::data(m_rtvHandles),
		FALSE, m_dsvHandle.get()
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
}
