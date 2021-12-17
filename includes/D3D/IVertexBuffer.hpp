#ifndef __I_VERTEX_BUFFER_HPP__
#define __I_VERTEX_BUFFER_HPP__
#include <D3DHeaders.hpp>

class IVertexBuffer {
public:
	virtual ~IVertexBuffer() = default;

	virtual D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferRef() noexcept = 0;
};
#endif
