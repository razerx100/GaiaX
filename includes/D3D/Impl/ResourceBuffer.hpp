#ifndef __RESOURCE_BUFFER_HPP__
#define __RESOURCE_BUFFER_HPP__
#include <IResourceBuffer.hpp>
#include <IUploadBuffer.hpp>
#include <IGPUBuffer.hpp>
#include <vector>
#include <memory>

class ResourceBuffer : public IResourceBuffer {
public:
	ResourceBuffer(BufferType type);

	size_t AddData(const void* source, size_t bufferSize) override;
	void CreateBuffer(ID3D12Device* device) override;
	void CopyData() noexcept override;
	void RecordUpload(ID3D12GraphicsCommandList* copyList) override;
	void ReleaseUploadBuffer() override;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUHandle() const noexcept override;

private:
	struct BufferData {
		const void* data;
		size_t size;
		size_t offset;
	};

private:
	std::unique_ptr<IUploadBuffer> m_pUploadBuffer;
	std::unique_ptr<IGPUBuffer> m_pGPUBuffer;
	size_t m_currentMemoryOffset;
	std::vector<BufferData> m_uploadBufferData;
	BufferType m_type;
};
#endif
