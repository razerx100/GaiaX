#include <D3DExternalBuffer.hpp>

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
{}

void D3DExternalTexture::Create(
	std::uint32_t width, std::uint32_t height, ExternalFormat format, ExternalTexture2DType type,
	bool copySrc, bool copyDst
) {

}

void D3DExternalTexture::Destroy() noexcept
{

}
