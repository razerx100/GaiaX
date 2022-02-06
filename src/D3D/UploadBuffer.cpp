#include <UploadBuffer.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>

std::uint8_t* UploadBuffer::GetCPUHandle() const noexcept {
	return m_cpuHandle;
}

ID3D12Resource* UploadBuffer::GetBuffer() const noexcept {
	return m_uploadBuffer.Get();
}

void UploadBuffer::MapBuffer() {
	HRESULT hr;
	D3D_THROW_FAILED(
		hr,
		m_uploadBuffer->Map(0u, nullptr, reinterpret_cast<void**>(&m_cpuHandle))
	);
}
