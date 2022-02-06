#include <D3DBuffer.hpp>

ID3D12Resource* D3DBuffer::Get() const noexcept {
	return m_pBuffer.Get();
}

ID3D12Resource** D3DBuffer::GetAddress() noexcept {
	return m_pBuffer.GetAddressOf();
}

ID3D12Resource** D3DBuffer::ReleaseAndGetAddress() noexcept {
	return m_pBuffer.ReleaseAndGetAddressOf();
}
