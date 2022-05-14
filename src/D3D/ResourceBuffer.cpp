#include <ResourceBuffer.hpp>
#include <HeapManager.hpp>
#include <Gaia.hpp>
#include <ranges>
#include <algorithm>
#include <D3DHelperFunctions.hpp>

D3DGPUSharedAddress ResourceBuffer::AddDataAndGetSharedAddress(
	const void* data, size_t bufferSize,
	bool alignment256
) noexcept {

	D3DGPUSharedAddress sharedAddress =
		std::make_shared<_SharedAddress<D3D12_GPU_VIRTUAL_ADDRESS>>();
	BufferData bufferData = { data, bufferSize, 0u };

	if (alignment256)
		m_align256Data.emplace_back(sharedAddress, bufferData);
	else
		m_align4Data.emplace_back(sharedAddress, bufferData);

	return sharedAddress;
}

void ResourceBuffer::SetGPUVirtualAddressToBuffers() noexcept {
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_pGPUBuffer->Get()->GetGPUVirtualAddress();

	for (auto& [sharedAddress256, bufferData256] : m_align256Data)
		*sharedAddress256 = gpuAddress + bufferData256.offset;

	for (auto& [sharedAddress4, bufferData4] : m_align4Data)
		*sharedAddress4 = gpuAddress + bufferData4.offset;
}

void ResourceBuffer::AcquireBuffers() {
	auto [gpuBuffer, uploadBuffer] =
		Gaia::heapManager->AddBuffer(ConfigureBufferSizeAndAllocations());

	m_pGPUBuffer = gpuBuffer;
	m_pUploadBuffer = uploadBuffer;
}

void ResourceBuffer::CopyData() noexcept {
	for (auto& [sharedAddress256, bufferData256] : m_align256Data)
		memcpy(
			m_pUploadBuffer->GetCPUHandle() + bufferData256.offset,
			bufferData256.data, bufferData256.size
		);

	for (auto& [sharedAddress4, bufferData4] : m_align4Data)
		memcpy(
			m_pUploadBuffer->GetCPUHandle() + bufferData4.offset,
			bufferData4.data, bufferData4.size
		);
}

void ResourceBuffer::ReleaseUploadBuffer() {
	m_pUploadBuffer.reset();
	m_align256Data = std::vector<AddressAndData>();
	m_align4Data = std::vector<AddressAndData>();
}

size_t ResourceBuffer::ConfigureBufferSizeAndAllocations() noexcept {
	size_t memoryOffset = 0u;

	auto predicate256 = [](const AddressAndData& data1, const AddressAndData& data2) {
		return data1.second.size % 256u < data2.second.size % 256u;
	};
	std::ranges::sort(m_align256Data, predicate256);

	auto predicate4 = [](const AddressAndData& data1, const AddressAndData& data2) {
		return data1.second.size < data2.second.size;
	};
	std::ranges::sort(m_align4Data, predicate4);

	std::int64_t largestMemoryIndex4 = static_cast<std::int64_t>(std::size(m_align4Data)) - 1;

	// Allocate Buffers larger than 256bytes with 4bytes alignment
	for (; largestMemoryIndex4 >= 0u; --largestMemoryIndex4) {
		BufferData& align4Data = m_align4Data[largestMemoryIndex4].second;

		if (align4Data.size < 256u)
			break;

		memoryOffset = Align(memoryOffset, 4u);
		align4Data.offset = memoryOffset;
	}

	// Allocate 4bytes aligned buffers between the space left from previous allocation and
	// next alignment
	std::int64_t smallestMemoryIndex4 = 0u;

	size_t emptySpace = Align(memoryOffset, 256u);
	emptySpace = emptySpace - memoryOffset;
	AllocateSmall4(memoryOffset, emptySpace, smallestMemoryIndex4, largestMemoryIndex4);

	// Allocate Buffers with 256bytes alignments and try to allocate 4bytes aligned buffers
	// between the remained space and next alignment
	for (size_t index = 0u; index < std::size(m_align256Data); ++index) {
		emptySpace = 0u;

		size_t oldOffset = memoryOffset;
		memoryOffset = Align(memoryOffset, 256u);
		emptySpace = memoryOffset - oldOffset;

		BufferData& align256Data = m_align256Data[index].second;
		align256Data.offset = memoryOffset;
		memoryOffset += align256Data.size;

		// Allocate smaller 4bytes aligned buffers in the remained space
		AllocateSmall4(memoryOffset, emptySpace, smallestMemoryIndex4, largestMemoryIndex4);
	}

	// Allocate the rest of the 4bytes aligned buffers
	for (; smallestMemoryIndex4 <= largestMemoryIndex4; ++smallestMemoryIndex4) {
		memoryOffset = Align(memoryOffset, 4u);

		BufferData& align4Data = m_align4Data[smallestMemoryIndex4].second;
		align4Data.offset = memoryOffset;
		memoryOffset += align4Data.size;
	}

	return memoryOffset;
}

void ResourceBuffer::AllocateSmall4(
	size_t& offset, size_t allocationBudget,
	std::int64_t& smallestMemoryIndex, std::int64_t largestMemoryIndex
) noexcept {
	std::int64_t emptySpace = static_cast<std::int64_t>(allocationBudget);

	for (; smallestMemoryIndex <= largestMemoryIndex; ++smallestMemoryIndex) {
		BufferData& align4Data = m_align4Data[smallestMemoryIndex].second;

		if (emptySpace < static_cast<std::int64_t>(align4Data.size))
			break;

		offset = Align(offset, 4u);
		align4Data.offset = offset;
		emptySpace -= Align(align4Data.size, 4u);
	}
}
