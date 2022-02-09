#ifndef __I_RESOURCE_BUFFER_HPP__
#define __I_RESOURCE_BUFFER_HPP__
#include <D3DHeaders.hpp>

enum class BufferType;

class IResourceBuffer {
public:
	virtual ~IResourceBuffer() = default;

	[[nodiscard]]
	virtual size_t AddDataAndGetOffset(
		const void* data, size_t bufferSize,
		size_t alignment = 4u
	) noexcept = 0;

	[[nodiscard]]
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const noexcept = 0;

	virtual void AcquireBuffers() = 0;
	virtual void CopyData() noexcept = 0;
	virtual void ReleaseUploadBuffer() = 0;
};

IResourceBuffer* CreateResourceBufferInstance(BufferType type);
#endif
