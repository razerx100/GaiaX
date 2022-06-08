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
	ResourceBuffer(size_t alignment) noexcept;

	[[nodiscard]]
	D3DGPUSharedAddress AddDataAndGetSharedAddress(
		std::unique_ptr<std::uint8_t> sourceHandle, size_t bufferSize
	) noexcept;

	void SetGPUVirtualAddressToBuffers() const noexcept;
	void AcquireBuffers();
	void CopyData() noexcept;
	void ReleaseUploadBuffer() noexcept;

private:
	struct BufferData {
		size_t size;
		size_t offset;
	};

private:
	std::vector<D3DGPUSharedAddress> m_sharedAddresses;
	std::vector<BufferData> m_bufferData;
	std::vector<std::unique_ptr<std::uint8_t>> m_sourceHandles;
	std::shared_ptr<D3DBuffer> m_pGPUBuffer;
	std::shared_ptr<UploadBuffer> m_pUploadBuffer;

	const size_t m_alignment;
};
#endif
