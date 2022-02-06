#include <UploadBuffer.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>

UploadBuffer::UploadBuffer() : m_cpuHandle(nullptr) {
	m_uploadBuffer = std::make_shared<D3DBuffer>();
}

std::uint8_t* UploadBuffer::GetCPUHandle() const noexcept {
	return m_cpuHandle;
}

std::shared_ptr<D3DBuffer> UploadBuffer::GetBuffer() const noexcept {
	return m_uploadBuffer;
}

void UploadBuffer::MapBuffer() {
	HRESULT hr;
	D3D_THROW_FAILED(
		hr,
		m_uploadBuffer->Get()->Map(0u, nullptr, reinterpret_cast<void**>(&m_cpuHandle))
	);
}
