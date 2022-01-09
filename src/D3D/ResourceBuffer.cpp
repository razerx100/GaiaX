#include <ResourceBuffer.hpp>
#include <UploadBuffer.hpp>
#include <GPUBuffer.hpp>
#include <d3dx12.h>
#include <VenusInstance.hpp>
#include <functional>

ResourceBuffer::ResourceBuffer()
	: m_currentMemoryOffset(0u) {

	m_pUploadBuffer = std::make_unique<UploadBuffer>();
	m_pGPUBuffer = std::make_unique<GPUBuffer>();
}

void ResourceBuffer::CreateBuffer(ID3D12Device* device, size_t bufferSize) {
	m_pUploadBuffer->CreateBuffer(device, bufferSize);
	m_pGPUBuffer->CreateBuffer(device, bufferSize);
}

void ResourceBuffer::CopyData(const void* source, size_t bufferSize) noexcept {
	std::memcpy(m_pUploadBuffer->GetCPUHandle() + m_currentMemoryOffset, source, bufferSize);
	m_currentMemoryOffset += bufferSize;
}

void ResourceBuffer::RecordUpload(ID3D12GraphicsCommandList* copyList, BufferType type) {
	copyList->CopyResource(m_pGPUBuffer->GetBuffer(), m_pUploadBuffer->GetBuffer());

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pGPUBuffer->GetBuffer(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		type == BufferType::Vertex ?
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER : D3D12_RESOURCE_STATE_INDEX_BUFFER
	);
	copyList->ResourceBarrier(1u, &barrier);
}

void ResourceBuffer::ReleaseUploadBuffer() {
	m_pUploadBuffer.reset();
}

D3D12_GPU_VIRTUAL_ADDRESS ResourceBuffer::GetGPUHandle() const noexcept {
	return m_pGPUBuffer->GetBuffer()->GetGPUVirtualAddress();
}
