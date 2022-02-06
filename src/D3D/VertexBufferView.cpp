#include <VertexBufferView.hpp>

VertexBufferView::VertexBufferView(
	ComPtr<ID3D12Resource> vertexBuffer,
	size_t bufferSize, size_t strideSize
)
	: m_vertexBuffer(vertexBuffer), m_bufferView{} {

	m_bufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_bufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	m_bufferView.StrideInBytes = static_cast<UINT>(strideSize);
}

const D3D12_VERTEX_BUFFER_VIEW* VertexBufferView::GetBufferViewRef() const noexcept {
	return &m_bufferView;
}
