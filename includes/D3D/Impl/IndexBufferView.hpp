#ifndef __INDEX_BUFFER_VIEW_HPP__
#define __INDEX_BUFFER_VIEW_HPP__
#include <IIndexBufferView.hpp>

class IndexBufferView : public IIndexBufferView {
public:
	IndexBufferView(
		ComPtr<ID3D12Resource> indexBuffer,
		size_t bufferSize
	);

	const D3D12_INDEX_BUFFER_VIEW* GetBufferViewRef() const noexcept override;

private:
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_bufferView;
};
#endif