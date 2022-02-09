#ifndef __RESOURCE_BUFFER_HPP__
#define __RESOURCE_BUFFER_HPP__
#include <IResourceBuffer.hpp>
#include <D3DBuffer.hpp>
#include <IUploadBuffer.hpp>
#include <vector>
#include <memory>

class ResourceBuffer : public IResourceBuffer {
public:
	ResourceBuffer(BufferType type);

	[[nodiscard]]
	size_t AddDataAndGetOffset(
		const void* data, size_t bufferSize,
		size_t alignment = 4u
	) noexcept override;

	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const noexcept override;

	void CreateBuffer() override;
	void CopyData() noexcept override;
	void ReleaseUploadBuffer() override;

private:
	struct BufferData {
		const void* data;
		size_t size;
		size_t offset;
	};

private:
	BufferType m_type;
	size_t m_currentOffset;
	std::vector<BufferData> m_bufferData;
	std::shared_ptr<D3DBuffer> m_pGPUBuffer;
	std::shared_ptr<IUploadBuffer> m_pUploadBuffer;
};
#endif
