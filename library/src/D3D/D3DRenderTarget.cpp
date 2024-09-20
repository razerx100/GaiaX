#include <D3DRenderTarget.hpp>
#include <D3DResourceBarrier.hpp>

// Render Target
RenderTarget::~RenderTarget() noexcept
{
	if (m_descriptorIndex != std::numeric_limits<UINT>::max())
		m_rtvHeap->FreeDescriptor(m_descriptorIndex);
}

void RenderTarget::Create(ComPtr<ID3D12Resource>&& renderTarget)
{
	m_renderTarget = std::move(renderTarget);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc
	{
		.Format        = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
		.Texture2D     = D3D12_TEX2D_RTV
		{
			.MipSlice = 0u
		}
	};

	if (m_descriptorIndex != std::numeric_limits<UINT>::max())
		m_rtvHeap->CreateRTV(m_renderTarget.Get(), rtvDesc, m_descriptorIndex);
	else
		m_descriptorIndex = m_rtvHeap->CreateRTV(m_renderTarget.Get(), rtvDesc);
}

void RenderTarget::ToRenderState(const D3DCommandList& commandList) const noexcept
{
	D3DResourceBarrier().AddBarrier(
		ResourceBarrierBuilder{}.Transition(
			m_renderTarget.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
		)
	).RecordBarriers(commandList.Get());
}

void RenderTarget::ToPresentState(const D3DCommandList& commandList) const noexcept
{
	D3DResourceBarrier().AddBarrier(
		ResourceBarrierBuilder{}.Transition(
			m_renderTarget.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
		)
	).RecordBarriers(commandList.Get());
}

void RenderTarget::Set(
	const D3DCommandList& commandList, const std::array<float, 4u>& clearColour,
	D3D12_CPU_DESCRIPTOR_HANDLE const* dsvHandle
) const {
	ID3D12GraphicsCommandList* cmdList          = commandList.Get();
	const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUHandle(m_descriptorIndex);

	cmdList->ClearRenderTargetView(rtvHandle, std::data(clearColour), 0u, nullptr);

	// If a valid dsv is provided, it has to be cleared beforehand.
	cmdList->OMSetRenderTargets(1u, &rtvHandle, FALSE, dsvHandle);
}
