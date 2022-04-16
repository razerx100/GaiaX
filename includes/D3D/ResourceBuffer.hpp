#ifndef RESOURCE_BUFFER_HPP_
#define RESOURCE_BUFFER_HPP_
#include <D3DBuffer.hpp>
#include <UploadBuffer.hpp>
#include <SharedAddress.hpp>
#include <deque>
#include <vector>
#include <memory>

using D3DGPUSharedAddress = std::shared_ptr<_SharedAddress<D3D12_GPU_VIRTUAL_ADDRESS>>;

class ResourceBuffer {
public:
	[[nodiscard]]
	D3DGPUSharedAddress AddDataAndGetSharedAddress(
		const void* data, size_t bufferSize,
		bool alignment256 = false
	) noexcept;

	void SetGPUVirtualAddressToBuffers() noexcept;
	void AcquireBuffers();
	void CopyData() noexcept;
	void ReleaseUploadBuffer();

private:
	struct BufferData {
		const void* data;
		size_t size;
		size_t offset;
	};

private:
	using BufferDataIter = std::vector<BufferData>::iterator;

	size_t ConfigureBufferSizeAndAllocations() noexcept;

	void FillWithIterator(
		std::deque<BufferDataIter>& dest,
		std::vector<BufferData>& source
	) const noexcept;
	void Align256(size_t& offset, size_t& emptySpace) const noexcept;
	void AllocateSmall4(
		std::deque<BufferDataIter>& align4,
		size_t& offset, size_t allocationBudget
	) const noexcept;

private:
	std::vector<BufferData> m_bufferData256;
	std::vector<BufferData> m_bufferData4;
	std::vector<D3DGPUSharedAddress> m_sharedGPUAddress256;
	std::vector<D3DGPUSharedAddress> m_sharedGPUAddress4;
	std::shared_ptr<D3DBuffer> m_pGPUBuffer;
	std::shared_ptr<UploadBuffer> m_pUploadBuffer;
};
#endif
