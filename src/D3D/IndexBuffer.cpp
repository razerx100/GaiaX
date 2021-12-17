#include <IndexBuffer.hpp>
#include <cstring>

IndexBuffer::IndexBuffer(
	ID3D12Device* device, const std::vector<std::uint16_t>& vertices
) : m_indexBufferView{}, m_indexCount(vertices.size()) {

	std::uint64_t strideSize = sizeof(std::uint16_t);
	std::uint64_t bufferSize = vertices.size() * strideSize;

	m_buffer.CreateBuffer(device, bufferSize);
	std::memcpy(m_buffer.GetCPUHandle(), vertices.data(), bufferSize);

	m_indexBufferView.BufferLocation = m_buffer.GetGPUHandle();
	m_indexBufferView.SizeInBytes = bufferSize;
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
}

D3D12_INDEX_BUFFER_VIEW* IndexBuffer::GetIndexBufferRef() noexcept {
	return &m_indexBufferView;
}

std::uint64_t IndexBuffer::GetIndexCount() const noexcept {
	return m_indexCount;
}
