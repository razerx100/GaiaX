#include <VertexBufferView.hpp>

VertexBufferView::VertexBufferView(
	std::shared_ptr<D3DBuffer> vertexBuffer,
	size_t bufferSize, size_t strideSize
)
	: m_vertexBuffer(vertexBuffer), m_bufferView{} {

	m_bufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	m_bufferView.StrideInBytes = static_cast<UINT>(strideSize);
}

const D3D12_VERTEX_BUFFER_VIEW* VertexBufferView::GetBufferViewRef() const noexcept {
	return &m_bufferView;
}

void VertexBufferView::SetGPUVirtualAddress() noexcept {
	m_bufferView.BufferLocation = m_vertexBuffer->Get()->GetGPUVirtualAddress();
}
