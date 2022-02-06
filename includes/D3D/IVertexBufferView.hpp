#ifndef __I_VERTEX_BUFFER_VIEW_HPP__
#define __I_VERTEX_BUFFER_VIEW_HPP__
#include <D3DHeaders.hpp>

class IVertexBufferView {
public:
	virtual ~IVertexBufferView() = default;

	virtual const D3D12_VERTEX_BUFFER_VIEW* GetBufferViewRef() const noexcept = 0;
};
#endif
