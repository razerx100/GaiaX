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
	[[maybe_unused]] bool copySrc, [[maybe_unused]] bool copyDst
) {
	D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE;

	if (type == ExternalTexture2DType::RenderTarget)
	{
		resourceFlag   = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		m_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	else if (type == ExternalTexture2DType::Depth || type == ExternalTexture2DType::Stencil)
	{
		resourceFlag   = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		m_currentState = D3D12_RESOURCE_STATE_DEPTH_READ;
	}

	m_texture.Create2D(
		width, height, 1u, GetDxgiFormat(format), m_currentState, resourceFlag
	);
}

ResourceBarrierBuilder D3DExternalTexture::TransitionState(D3D12_RESOURCE_STATES newState) noexcept
{
	ResourceBarrierBuilder barrierBuilder{};

	barrierBuilder.Transition(m_texture.Get(), m_currentState, newState);

	m_currentState = newState;

	return barrierBuilder;
}
