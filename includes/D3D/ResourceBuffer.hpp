#ifndef RESOURCE_BUFFER_HPP_
#define RESOURCE_BUFFER_HPP_
#include <D3DBuffer.hpp>
#include <UploadBuffer.hpp>
#include <SharedAddress.hpp>
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

	using AddressAndData = std::pair<D3DGPUSharedAddress, BufferData>;

private:
	size_t ConfigureBufferSizeAndAllocations() noexcept;

	void AllocateSmall4(
		size_t& offset, size_t allocationBudget,
		std::int64_t& smallestMemoryIndex, std::int64_t largestMemoryIndex
	) noexcept;

private:
	std::vector<AddressAndData> m_align256Data;
	std::vector<AddressAndData> m_align4Data;
	std::shared_ptr<D3DBuffer> m_pGPUBuffer;
	std::shared_ptr<UploadBuffer> m_pUploadBuffer;
};
#endif
