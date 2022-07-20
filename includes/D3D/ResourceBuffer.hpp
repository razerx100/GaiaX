#ifndef RESOURCE_BUFFER_HPP_
#define RESOURCE_BUFFER_HPP_
#include <D3DBuffer.hpp>
#include <UploadBuffer.hpp>
#include <vector>
#include <memory>
#include <GaiaDataTypes.hpp>

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
	D3DBufferShared m_pGPUBuffer;
	UploadBufferShared m_pUploadBuffer;

	const size_t m_alignment;
};
#endif
