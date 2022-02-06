#ifndef __VERTEX_BUFFER_VIEW_HPP__
#define __VERTEX_BUFFER_VIEW_HPP__
#include <IVertexBufferView.hpp>
#include <D3DBuffer.hpp>
#include <memory>

class VertexBufferView : public IVertexBufferView {
public:
	VertexBufferView(
		std::shared_ptr<D3DBuffer> vertexBuffer,
		size_t bufferSize, size_t strideSize
	);

	const D3D12_VERTEX_BUFFER_VIEW* GetBufferViewRef() const noexcept override;

	void SetGPUVirtualAddress() noexcept override;

private:
	std::shared_ptr<D3DBuffer> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_bufferView;
};
#endif
