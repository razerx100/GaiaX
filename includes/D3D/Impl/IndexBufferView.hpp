#ifndef __INDEX_BUFFER_VIEW_HPP__
#define __INDEX_BUFFER_VIEW_HPP__
#include <IIndexBufferView.hpp>
#include <D3DBuffer.hpp>
#include <memory>

class IndexBufferView : public IIndexBufferView {
public:
	IndexBufferView(
		std::shared_ptr<D3DBuffer> indexBuffer,
		size_t bufferSize
	);

	const D3D12_INDEX_BUFFER_VIEW* GetBufferViewRef() const noexcept override;

	void SetGPUVirtualAddress() noexcept override;

private:
	std::shared_ptr<D3DBuffer> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_bufferView;
};
#endif