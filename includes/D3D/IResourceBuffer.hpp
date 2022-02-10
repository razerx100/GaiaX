#ifndef __I_RESOURCE_BUFFER_HPP__
#define __I_RESOURCE_BUFFER_HPP__
#include <D3DHeaders.hpp>
#include <memory>
#include <SharedAddress.hpp>

enum class BufferType;

using D3DGPUSharedAddress = std::shared_ptr<_SharedAddress<D3D12_GPU_VIRTUAL_ADDRESS>>;

class IResourceBuffer {
public:
	virtual ~IResourceBuffer() = default;

	[[nodiscard]]
	virtual D3DGPUSharedAddress AddDataAndGetSharedAddress(
		const void* data, size_t bufferSize,
		size_t alignment = 4u
	) noexcept = 0;

	virtual void SetGPUVirtualAddressToBuffers() noexcept = 0;
	virtual void AcquireBuffers() = 0;
	virtual void CopyData() noexcept = 0;
	virtual void ReleaseUploadBuffer() = 0;
};

IResourceBuffer* CreateResourceBufferInstance(BufferType type);
#endif
