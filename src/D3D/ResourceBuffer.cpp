#include <ResourceBuffer.hpp>
#include <UploadBuffer.hpp>
#include <GPUBuffer.hpp>
#include <d3dx12.h>
#include <VenusInstance.hpp>
#include <functional>
#include <CRSMath.hpp>

ResourceBuffer::ResourceBuffer(BufferType type)
	: m_currentMemoryOffset(0u), m_type(type) {

	m_pUploadBuffer = std::make_unique<UploadBuffer>();
	m_pGPUBuffer = std::make_unique<GPUBuffer>();
}

void ResourceBuffer::CreateBuffer(ID3D12Device* device) {
	size_t alignedSize = Ceres::Math::Align(m_currentMemoryOffset, 64_KB);

	m_pUploadBuffer->CreateBuffer(device, alignedSize);
	m_pGPUBuffer->CreateBuffer(device, alignedSize);
}

void ResourceBuffer::CopyData() noexcept {
	for(auto& bufferData : m_uploadBufferData)
		std::memcpy(
			m_pUploadBuffer->GetCPUHandle() + bufferData.offset,
			bufferData.data, bufferData.size
		);
}

void ResourceBuffer::RecordUpload(ID3D12GraphicsCommandList* copyList) {
	copyList->CopyResource(m_pGPUBuffer->GetBuffer(), m_pUploadBuffer->GetBuffer());

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pGPUBuffer->GetBuffer(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		m_type == BufferType::Vertex ?
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER : D3D12_RESOURCE_STATE_INDEX_BUFFER
	);
	copyList->ResourceBarrier(1u, &barrier);
}

void ResourceBuffer::ReleaseUploadBuffer() {
	m_uploadBufferData = std::vector<BufferData>();
	m_pUploadBuffer.reset();
}

D3D12_GPU_VIRTUAL_ADDRESS ResourceBuffer::GetGPUHandle() const noexcept {
	return m_pGPUBuffer->GetBuffer()->GetGPUVirtualAddress();
}

size_t ResourceBuffer::AddData(const void* source, size_t bufferSize) {
	m_uploadBufferData.emplace_back(source, bufferSize, m_currentMemoryOffset);
	m_currentMemoryOffset += bufferSize;

	return m_uploadBufferData.back().offset;
}
