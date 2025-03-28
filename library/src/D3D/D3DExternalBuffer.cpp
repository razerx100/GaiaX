#include <utility>
#include <D3DExternalBuffer.hpp>
#include <D3DExternalFormatMap.hpp>

// External Buffer
D3DExternalBuffer::D3DExternalBuffer(
	ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType,
	D3D12_RESOURCE_STATES resourceState, D3D12_RESOURCE_FLAGS bufferFlag
) : m_buffer{ device, memoryManager, memoryType }, m_resourceState{ resourceState },
	m_bufferFlag{ bufferFlag }
{}

void D3DExternalBuffer::Create(size_t bufferSize)
{
	m_buffer.Create(static_cast<UINT64>(bufferSize), m_resourceState, m_bufferFlag);
}

// External Texture
D3DExternalTexture::D3DExternalTexture(ID3D12Device* device, MemoryManager* memoryManager)
	: m_texture{ device, memoryManager, D3D12_HEAP_TYPE_DEFAULT },
	m_currentState{ D3D12_RESOURCE_STATE_COMMON }, m_clearValue{ .Format = DXGI_FORMAT_UNKNOWN },
	m_renderingAttachment{}
{}

void D3DExternalTexture::Create(
	std::uint32_t width, std::uint32_t height, ExternalFormat format, ExternalTexture2DType type,
	[[maybe_unused]] const ExternalTextureCreationFlags& creationFlags
) {
	D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE;
	const DXGI_FORMAT resourceFormat  = GetDxgiFormat(format);

	const bool isRenderTarget = type == ExternalTexture2DType::RenderTarget;
	const bool isDepthStencilTarget =
		type == ExternalTexture2DType::Depth || type == ExternalTexture2DType::Stencil;

	if (isRenderTarget)
		resourceFlag = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	else if (isDepthStencilTarget)
		resourceFlag = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	m_clearValue.Format = resourceFormat;

	// Don't need msaa? For now at least.
	m_texture.Create2D(
		width, height, 1u, resourceFormat, m_currentState, resourceFlag, false, &m_clearValue
	);

	if (isRenderTarget)
		m_renderingAttachment.CreateRTV(m_texture.Get(), resourceFormat);
	else if (isDepthStencilTarget)
		m_renderingAttachment.CreateDSV(m_texture.Get(), resourceFormat);
}

void D3DExternalTexture::SetRenderTargetClearColour(const std::array<float, 4u>& colour) noexcept
{
	m_clearValue.Color[0] = colour[0];
	m_clearValue.Color[1] = colour[1];
	m_clearValue.Color[2] = colour[2];
	m_clearValue.Color[3] = colour[3];
}

void D3DExternalTexture::AddDSVFlag(D3D12_DSV_FLAGS dsvFlag) noexcept
{
	m_renderingAttachment.AddDSVFlag(dsvFlag);
}

void D3DExternalTexture::SetDSVFlag(D3D12_DSV_FLAGS dsvFlag) noexcept
{
	m_renderingAttachment.SetDSVFlag(dsvFlag);
}

void D3DExternalTexture::SetDepthStencilClearColour(
	const D3D12_DEPTH_STENCIL_VALUE& depthStencilColour
) noexcept {
	m_clearValue.DepthStencil = depthStencilColour;
}

void D3DExternalTexture::SetDepthClearColour(float depthColour) noexcept
{
	m_clearValue.DepthStencil.Depth = depthColour;
}

void D3DExternalTexture::SetStencilClearColour(UINT8 stencilColour) noexcept
{
	m_clearValue.DepthStencil.Stencil = stencilColour;
}

void D3DExternalTexture::SetAttachmentHeap(D3DReusableDescriptorHeap* attachmentHeap) noexcept
{
	m_renderingAttachment.SetAttachmentHeap(attachmentHeap);
}

void D3DExternalTexture::Recreate(ExternalTexture2DType type)
{
	D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE;

	if (type == ExternalTexture2DType::RenderTarget)
		resourceFlag = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	else if (type == ExternalTexture2DType::Depth || type == ExternalTexture2DType::Stencil)
		resourceFlag = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	m_texture.Recreate2D(m_currentState, resourceFlag, &m_clearValue);
}
