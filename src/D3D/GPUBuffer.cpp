#include <GPUBuffer.hpp>
#include <d3dx12.h>
#include <D3DThrowMacros.hpp>

void GPUBuffer::CreateBuffer(ID3D12Device* device, size_t bufferSize) {
	CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC rsDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	HRESULT hr;
	D3D_THROW_FAILED(
		hr,
		device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&rsDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			__uuidof(ID3D12Resource),
			&m_gpuBuffer
		)
	);

	m_bufferSize = bufferSize;
}

D3D12_GPU_VIRTUAL_ADDRESS GPUBuffer::GetGPUHandle() const noexcept {
	return m_gpuBuffer->GetGPUVirtualAddress();
}

size_t GPUBuffer::GetBufferSize() const noexcept {
	return m_bufferSize;
}

ID3D12Resource* GPUBuffer::GetBuffer() const noexcept {
	return m_gpuBuffer.Get();
}
