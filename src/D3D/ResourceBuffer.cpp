#include <ResourceBuffer.hpp>
#include <IHeapManager.hpp>
#include <CRSMath.hpp>
#include <InstanceManager.hpp>

ResourceBuffer::ResourceBuffer(BufferType type)
	: m_type(type), m_currentOffset(0u) {}

D3DGPUSharedAddress ResourceBuffer::AddDataAndGetSharedAddress(
	const void* data, size_t bufferSize,
	size_t alignment
) noexcept {
	m_currentOffset = Ceres::Math::Align(m_currentOffset, alignment);

	m_sharedGPUAddresses.emplace_back(
		std::make_shared<_SharedAddress<D3D12_GPU_VIRTUAL_ADDRESS>>()
	);
	m_bufferData.emplace_back(data, bufferSize, m_currentOffset);
	m_currentOffset += bufferSize;

	return m_sharedGPUAddresses.back();
}

void ResourceBuffer::SetGPUVirtualAddressToBuffers() noexcept {
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_pGPUBuffer->Get()->GetGPUVirtualAddress();

	for (size_t index = 0u; index < m_sharedGPUAddresses.size(); ++index)
		*m_sharedGPUAddresses[index] = gpuAddress + m_bufferData[index].offset;
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
