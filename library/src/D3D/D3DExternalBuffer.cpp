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
	m_currentState{ D3D12_RESOURCE_STATE_COMMON }
{}

void D3DExternalTexture::Create(
	std::uint32_t width, std::uint32_t height, ExternalFormat format, ExternalTexture2DType type,
	[[maybe_unused]] bool copySrc, [[maybe_unused]] bool copyDst, const ExternalClearColour& clearColour
) {
	D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE;
	const DXGI_FORMAT resourceFormat  = GetDxgiFormat(format);
	// D3D12 needs a clear value during the resource creation, or it tanks the performance.
	D3D12_CLEAR_VALUE clearValue{};

	if (type == ExternalTexture2DType::RenderTarget)
	{
		resourceFlag = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		clearValue   = D3D12_CLEAR_VALUE
		{
			.Format = resourceFormat,
			.Color  = {
				clearColour.colour[0],
				clearColour.colour[1],
				clearColour.colour[2],
				clearColour.colour[3]
			}
		};
	}
	else if (type == ExternalTexture2DType::Depth)
	{
		resourceFlag = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		clearValue   = D3D12_CLEAR_VALUE
		{
			.Format       = resourceFormat,
			.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ .Depth = clearColour.depth }
		};
	}
	else if (type == ExternalTexture2DType::Stencil)
	{
		resourceFlag = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		clearValue   = D3D12_CLEAR_VALUE
		{
			.Format       = resourceFormat,
			.DepthStencil = D3D12_DEPTH_STENCIL_VALUE{ .Stencil = static_cast<UINT8>(clearColour.stencil) }
		};
	}

	// Don't need msaa? For now at least.
	m_texture.Create2D(
		width, height, 1u, resourceFormat, m_currentState, resourceFlag, false, &clearValue
	);
}

ResourceBarrierBuilder D3DExternalTexture::TransitionState(D3D12_RESOURCE_STATES newState) noexcept
{
	ResourceBarrierBuilder barrierBuilder{};

	barrierBuilder.Transition(m_texture.Get(), m_currentState, newState);

	m_currentState = newState;

	return barrierBuilder;
}
