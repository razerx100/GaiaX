#ifndef D3D_BUFFER_HPP_
#define D3D_BUFFER_HPP_
#include <D3DHeaders.hpp>

class D3DBuffer {
public:
	[[nodiscard]]
	ID3D12Resource* Get() const noexcept;
	ID3D12Resource** GetAddress() noexcept;
	ID3D12Resource** ReleaseAndGetAddress() noexcept;

private:
	ComPtr<ID3D12Resource> m_pBuffer;
};
#endif
