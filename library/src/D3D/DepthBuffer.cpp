#include <DepthBuffer.hpp>

DepthBuffer::~DepthBuffer() noexcept
{
	if (m_dsvHandleIndex != std::numeric_limits<UINT>::max())
		m_dsvHeap->FreeDescriptor(m_dsvHandleIndex);
}

void DepthBuffer::Create(UINT width, UINT height)
{
	const DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;

	D3D12_CLEAR_VALUE depthClearValue
	{
		.Format = depthFormat,
		.Color  = { 1.f, 0.f, 0.f, 0.f }
	};

	m_depthTexture.Create2D(
		width, height, 1u, depthFormat, D3D12_RESOURCE_STATE_DEPTH_WRITE,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, false, &depthClearValue
	);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
	{
		.Format        = depthFormat,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags         = D3D12_DSV_FLAG_NONE,
		.Texture2D     = D3D12_TEX2D_DSV
		{
			.MipSlice = 0u
		}
	};

	if (m_dsvHandleIndex != std::numeric_limits<UINT>::max())
		m_dsvHeap->CreateDSV(m_depthTexture.Get(), dsvDesc, m_dsvHandleIndex);
	else
		m_dsvHandleIndex = m_dsvHeap->CreateDSV(m_depthTexture.Get(), dsvDesc);
}

void DepthBuffer::ClearDSV(const D3DCommandList& commandList) const noexcept
{
	ID3D12GraphicsCommandList6* cmdList = commandList.Get();

	cmdList->ClearDepthStencilView(
		m_dsvHeap->GetCPUHandle(m_dsvHandleIndex), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0u, 0u, nullptr
	);
}
