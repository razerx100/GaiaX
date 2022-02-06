#ifndef __D3D_BUFFER_HPP__
#define __D3D_BUFFER_HPP__
#include <D3DHeaders.hpp>

class D3DBuffer {
public:
	ID3D12Resource* Get() const noexcept;
	ID3D12Resource** GetAddress() noexcept;
	ID3D12Resource** ReleaseAndGetAddress() noexcept;

private:
	ComPtr<ID3D12Resource> m_pBuffer;
};
#endif
