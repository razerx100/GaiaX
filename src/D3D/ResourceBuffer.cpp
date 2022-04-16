#include <ResourceBuffer.hpp>
#include <HeapManager.hpp>
#include <CRSMath.hpp>
#include <Gaia.hpp>
#include <algorithm>

D3DGPUSharedAddress ResourceBuffer::AddDataAndGetSharedAddress(
	const void* data, size_t bufferSize,
	bool alignment256
) noexcept {

	D3DGPUSharedAddress sharedAddress;

	if (alignment256) {
		m_sharedGPUAddress256.emplace_back(
			std::make_shared<_SharedAddress<D3D12_GPU_VIRTUAL_ADDRESS>>()
		);
		sharedAddress = m_sharedGPUAddress256.back();

		m_bufferData256.emplace_back(data, bufferSize, 0u);
	}
	else {
		m_sharedGPUAddress4.emplace_back(
			std::make_shared<_SharedAddress<D3D12_GPU_VIRTUAL_ADDRESS>>()
		);
		sharedAddress = m_sharedGPUAddress4.back();

		m_bufferData4.emplace_back(data, bufferSize, 0u);
	}

	return sharedAddress;
}

void ResourceBuffer::SetGPUVirtualAddressToBuffers() noexcept {
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_pGPUBuffer->Get()->GetGPUVirtualAddress();

	for (size_t index = 0u; index < m_sharedGPUAddress256.size(); ++index)
		*m_sharedGPUAddress256[index] = gpuAddress + m_bufferData256[index].offset;

	for (size_t index = 0u; index < m_sharedGPUAddress4.size(); ++index)
		*m_sharedGPUAddress4[index] = gpuAddress + m_bufferData4[index].offset;
}

void ResourceBuffer::AcquireBuffers() {
	auto [gpuBuffer, uploadBuffer] =
		Gaia::heapManager->AddBuffer(ConfigureBufferSizeAndAllocations());

	m_pGPUBuffer = gpuBuffer;
	m_pUploadBuffer = uploadBuffer;
}

void ResourceBuffer::CopyData() noexcept {
	for (auto& bufferData : m_bufferData256)
		memcpy(
			m_pUploadBuffer->GetCPUHandle() + bufferData.offset,
			bufferData.data, bufferData.size
		);

	for (auto& bufferData : m_bufferData4)
		memcpy(
			m_pUploadBuffer->GetCPUHandle() + bufferData.offset,
			bufferData.data, bufferData.size
		);
}

void ResourceBuffer::ReleaseUploadBuffer() {
	m_pUploadBuffer.reset();
	m_bufferData256 = std::vector<BufferData>();
	m_bufferData4 = std::vector<BufferData>();
}

size_t ResourceBuffer::ConfigureBufferSizeAndAllocations() noexcept {
	size_t memoryOffset = 0u;

	std::deque<BufferDataIter> sorted256Data;
	FillWithIterator(sorted256Data, m_bufferData256);

	auto predicate256 = [](
		BufferDataIter iter1,
		BufferDataIter iter2
		) {
			return (*iter1).size % 256u < (*iter2).size % 256u;
	};
	std::sort(sorted256Data.begin(), sorted256Data.end(), predicate256);

	std::deque<BufferDataIter> sorted4Data;
	FillWithIterator(sorted4Data, m_bufferData4);

	auto predicate4 = [](
		BufferDataIter iter1,
		BufferDataIter iter2
		) {
			return (*iter1).size < (*iter2).size;
	};
	std::sort(sorted4Data.begin(), sorted4Data.end(), predicate4);

	for (BufferData& bufferData = *sorted4Data.back();
		bufferData.size >= 256u;
		memoryOffset += bufferData.size, bufferData = *sorted4Data.back()) {
		memoryOffset = Ceres::Math::Align(memoryOffset, 4u);

		bufferData.offset = memoryOffset;
		sorted4Data.pop_back();
	}

	size_t emptySpace = Ceres::Math::Align(memoryOffset, 256u);
	emptySpace = emptySpace - memoryOffset;
	AllocateSmall4(sorted4Data, memoryOffset, emptySpace);

	emptySpace = 0u;

	while (!sorted256Data.empty()) {
		emptySpace = 0u;
		Align256(memoryOffset, emptySpace);

		BufferData& bufferData = *sorted256Data.front();
		bufferData.offset = memoryOffset;
		memoryOffset += bufferData.size;
		sorted256Data.pop_front();

		AllocateSmall4(sorted4Data, memoryOffset, emptySpace);
	}

	while (!sorted4Data.empty()) {
		memoryOffset = Ceres::Math::Align(memoryOffset, 4u);

		BufferData& bufferData = *sorted4Data.front();
		bufferData.offset = memoryOffset;
		memoryOffset += bufferData.size;
		sorted4Data.pop_front();
	}

	return memoryOffset;
}

void ResourceBuffer::FillWithIterator(
	std::deque<BufferDataIter>& dest,
	std::vector<BufferData>& source
) const noexcept {
	for (auto iterBegin = source.begin(); iterBegin != source.end(); ++iterBegin)
		dest.emplace_back(iterBegin);
}

void ResourceBuffer::Align256(size_t& offset, size_t& emptySpace) const noexcept {
	size_t oldOffset = offset;
	offset = Ceres::Math::Align(offset, 256u);
	emptySpace += offset - oldOffset;
}

void ResourceBuffer::AllocateSmall4(
	std::deque<BufferDataIter>& align4, size_t& offset, size_t allocationBudget
) const noexcept {
	std::int64_t emptySpace = static_cast<std::int64_t>(allocationBudget);

	for (BufferData& bufferData = *align4.front();
		emptySpace >= static_cast<std::int64_t>(bufferData.size);
		offset += bufferData.size, emptySpace -= bufferData.size,
		bufferData = *align4.front()) {
		offset = Ceres::Math::Align(offset, 4u);

		bufferData.offset = offset;
		align4.pop_front();
	}
}
