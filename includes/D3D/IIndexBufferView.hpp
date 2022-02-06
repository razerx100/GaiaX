#ifndef __I_INDEX_BUFFER_VIEW_HPP__
#define __I_INDEX_BUFFER_VIEW_HPP__
#include <D3DHeaders.hpp>

class IIndexBufferView {
public:
	virtual ~IIndexBufferView() = default;

	virtual const D3D12_INDEX_BUFFER_VIEW* GetBufferViewRef() const noexcept = 0;

	virtual void SetGPUVirtualAddress() noexcept = 0;
};
#endif
