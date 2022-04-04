#ifndef __RESOURCE_BUFFER_HPP__
#define __RESOURCE_BUFFER_HPP__
#include <IResourceBuffer.hpp>
#include <D3DBuffer.hpp>
#include <IUploadBuffer.hpp>
#include <deque>
#include <vector>
#include <memory>

class ResourceBuffer : public IResourceBuffer {
public:
	[[nodiscard]]
	D3DGPUSharedAddress AddDataAndGetSharedAddress(
		const void* data, size_t bufferSize,
		bool alignment256 = false
	) noexcept override;

	void SetGPUVirtualAddressToBuffers() noexcept override;
	void AcquireBuffers() override;
	void CopyData() noexcept override;
	void ReleaseUploadBuffer() override;

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
	std::shared_ptr<IUploadBuffer> m_pUploadBuffer;
};
#endif
