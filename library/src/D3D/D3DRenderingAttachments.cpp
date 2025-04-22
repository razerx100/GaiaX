#include <D3DRenderingAttachments.hpp>

namespace Gaia
{
// Rendering Attachment
RenderingAttachment::~RenderingAttachment() noexcept
{
	if (m_attachmentHeap && m_descriptorIndex != std::numeric_limits<UINT>::max())
		m_attachmentHeap->FreeDescriptor(m_descriptorIndex);
}

void RenderingAttachment::CreateRTV(ID3D12Resource* renderTarget, DXGI_FORMAT rtvFormat)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc
	{
		.Format        = rtvFormat,
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
		.Texture2D     = D3D12_TEX2D_RTV
		{
			.MipSlice = 0u
		}
	};

	if (m_descriptorIndex != std::numeric_limits<UINT>::max())
		m_attachmentHeap->CreateRTV(renderTarget, rtvDesc, m_descriptorIndex);
	else
		m_descriptorIndex = m_attachmentHeap->CreateRTV(renderTarget, rtvDesc);
}

void RenderingAttachment::CreateDSV(ID3D12Resource* depthStencilTarget, DXGI_FORMAT dsvFormat)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
	{
		.Format        = dsvFormat,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags         = static_cast<D3D12_DSV_FLAGS>(m_dsvFlags),
		.Texture2D     = D3D12_TEX2D_DSV
		{
			.MipSlice = 0u
		}
	};

	if (m_descriptorIndex != std::numeric_limits<UINT>::max())
		m_attachmentHeap->CreateDSV(depthStencilTarget, dsvDesc, m_descriptorIndex);
	else
		m_descriptorIndex = m_attachmentHeap->CreateDSV(depthStencilTarget, dsvDesc);
}
}
