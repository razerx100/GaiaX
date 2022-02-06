#ifndef __VERTEX_BUFFER_VIEW_HPP__
#define __VERTEX_BUFFER_VIEW_HPP__
#include <IVertexBufferView.hpp>

class VertexBufferView : public IVertexBufferView {
public:
	VertexBufferView(
		ComPtr<ID3D12Resource> vertexBuffer,
		size_t bufferSize, size_t strideSize
	);

	const D3D12_VERTEX_BUFFER_VIEW* GetBufferViewRef() const noexcept override;

private:
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_bufferView;
};
#endif
