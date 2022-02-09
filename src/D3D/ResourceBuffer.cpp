#include <ResourceBuffer.hpp>
#include <IHeapManager.hpp>
#include <CRSMath.hpp>
#include <InstanceManager.hpp>

ResourceBuffer::ResourceBuffer(BufferType type)
	: m_type(type), m_currentOffset(0u) {}

size_t ResourceBuffer::AddDataAndGetOffset(
	const void* data, size_t bufferSize,
	size_t alignment
) noexcept {
	m_currentOffset = Ceres::Math::Align(m_currentOffset, alignment);

	m_bufferData.emplace_back(data, bufferSize, m_currentOffset);

	m_currentOffset += bufferSize;

	return m_bufferData.back().offset;
}

D3D12_GPU_VIRTUAL_ADDRESS ResourceBuffer::GetGPUVirtualAddress() const noexcept {
	return m_pGPUBuffer->Get()->GetGPUVirtualAddress();
}

void ResourceBuffer::AcquireBuffers() {
	auto [gpuBuffer, uploadBuffer] =
		HeapManagerInst::GetRef()->AddBuffer(m_currentOffset, m_type);

	m_pGPUBuffer = gpuBuffer;
	m_pUploadBuffer = uploadBuffer;
}

void ResourceBuffer::CopyData() noexcept {
	for (auto& bufferData : m_bufferData)
		memcpy(
			m_pUploadBuffer->GetCPUHandle() + bufferData.offset,
			bufferData.data, bufferData.size
		);
}

void ResourceBuffer::ReleaseUploadBuffer() {
	m_pUploadBuffer.reset();
	m_bufferData = std::vector<BufferData>();
}
