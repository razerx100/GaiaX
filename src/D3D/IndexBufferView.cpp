#include <IndexBufferView.hpp>

IndexBufferView::IndexBufferView(
	std::shared_ptr<D3DBuffer> indexBuffer,
		size_t bufferSize
) : m_indexBuffer(indexBuffer), m_bufferView{} {

	m_bufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	m_bufferView.Format = DXGI_FORMAT_R16_UINT;
}

const D3D12_INDEX_BUFFER_VIEW* IndexBufferView::GetBufferViewRef() const noexcept {
	return &m_bufferView;
}

void IndexBufferView::SetGPUVirtualAddress() noexcept {
	m_bufferView.BufferLocation = m_indexBuffer->Get()->GetGPUVirtualAddress();
}
