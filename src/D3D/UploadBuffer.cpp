#include <UploadBuffer.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>

void UploadBuffer::CreateBuffer(ID3D12Device* device, size_t bufferSize) {
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC rsDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	HRESULT hr;
	D3D_THROW_FAILED(
		hr,
		device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&rsDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			__uuidof(ID3D12Resource),
			&m_uploadBuffer
		)
	);

	m_bufferSize = bufferSize;

	D3D_THROW_FAILED(
		hr,
		m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_cpuHandle))
	);
}

std::uint8_t* UploadBuffer::GetCPUHandle() const noexcept {
	return m_cpuHandle;
}

size_t UploadBuffer::GetBufferSize() const noexcept {
	return m_bufferSize;
}

ID3D12Resource* UploadBuffer::GetBuffer() const noexcept {
	return m_uploadBuffer.Get();
}
