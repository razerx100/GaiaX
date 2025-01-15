#include <D3DExternalBuffer.hpp>

D3DExternalBuffer::D3DExternalBuffer(
	ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType,
	D3D12_RESOURCE_STATES resourceState, D3D12_RESOURCE_FLAGS bufferFlag
) : m_buffer{ device, memoryManager, memoryType }, m_resourceState{ resourceState },
	m_bufferFlag{ bufferFlag }
{}

void D3DExternalBuffer::Create(size_t bufferSize)
{
	// For now, let's assume these buffers would only be used in the Graphics queue.
	m_buffer.Create(static_cast<UINT64>(bufferSize), m_resourceState, m_bufferFlag);
}
