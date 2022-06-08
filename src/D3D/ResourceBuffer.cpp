#include <ResourceBuffer.hpp>
#include <HeapManager.hpp>
#include <Gaia.hpp>
#include <ranges>
#include <algorithm>
#include <D3DHelperFunctions.hpp>

ResourceBuffer::ResourceBuffer(size_t alignment) noexcept : m_alignment(alignment) {}

D3DGPUSharedAddress ResourceBuffer::AddDataAndGetSharedAddress(
	std::unique_ptr<std::uint8_t> sourceHandle, size_t bufferSize
) noexcept {
	m_sourceHandles.emplace_back(std::move(sourceHandle));
	m_bufferData.emplace_back(bufferSize, 0u);

	D3DGPUSharedAddress sharedAddress =
		std::make_shared<_SharedAddress<D3D12_GPU_VIRTUAL_ADDRESS>>();

	m_sharedAddresses.emplace_back(sharedAddress);

	return sharedAddress;
}

void ResourceBuffer::SetGPUVirtualAddressToBuffers() const noexcept {
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_pGPUBuffer->Get()->GetGPUVirtualAddress();

	for (size_t index = 0u; index < std::size(m_bufferData); ++index)
		*m_sharedAddresses[index] = gpuAddress + m_bufferData[index].offset;
}

void ResourceBuffer::AcquireBuffers() {
	size_t offset = 0u;

	for (size_t index = 0u; index < std::size(m_bufferData); ++index) {
		offset = Align(offset, m_alignment);

		BufferData& bufferData = m_bufferData[index];
		bufferData.offset = offset;
		offset += bufferData.size;
	}

	auto [gpuBuffer, uploadBuffer] = Gaia::heapManager->AddBuffer(offset);

	m_pGPUBuffer = gpuBuffer;
	m_pUploadBuffer = uploadBuffer;
}

void ResourceBuffer::CopyData() noexcept {
	for (size_t index = 0u; index < std::size(m_bufferData); ++index) {
		auto& [bufferSize, bufferOffset] = m_bufferData[index];

		memcpy(
			m_pUploadBuffer->GetCPUHandle() + bufferOffset,
			m_sourceHandles[index].get(), bufferSize
		);
	}
}

void ResourceBuffer::ReleaseUploadBuffer() noexcept {
	m_pUploadBuffer.reset();
	m_sourceHandles = std::vector<std::unique_ptr<std::uint8_t>>();
	m_bufferData = std::vector<BufferData>();
}
