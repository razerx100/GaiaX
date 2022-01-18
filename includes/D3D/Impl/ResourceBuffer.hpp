#ifndef __RESOURCE_BUFFER_HPP__
#define __RESOURCE_BUFFER_HPP__
#include <IResourceBuffer.hpp>
#include <IUploadBuffer.hpp>
#include <IGPUBuffer.hpp>
#include <memory>

class ResourceBuffer : public IResourceBuffer {
public:
	ResourceBuffer();

	void CreateBuffer(ID3D12Device* device, size_t bufferSize) override;
	void CopyData(const void* source, size_t bufferSize) noexcept override;
	void RecordUpload(ID3D12GraphicsCommandList* copyList, BufferType type) override;
	void ReleaseUploadBuffer() override;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUHandle() const noexcept override;

private:
	std::unique_ptr<IUploadBuffer> m_pUploadBuffer;
	std::unique_ptr<IGPUBuffer> m_pGPUBuffer;
	size_t m_currentMemoryOffset;
};
#endif
