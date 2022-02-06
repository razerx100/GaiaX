#ifndef __RESOURCE_BUFFER_HPP__
#define __RESOURCE_BUFFER_HPP__
#include <IResourceBuffer.hpp>
#include <ID3DHeap.hpp>
#include <IUploadBuffer.hpp>
#include <vector>
#include <memory>

class ResourceBuffer : public IResourceBuffer {
public:
	ResourceBuffer();

	ComPtr<ID3D12Resource> AddBuffer(
		const void* source, size_t bufferSize, BufferType type
	) override;
	ComPtr<ID3D12Resource> AddTexture(const void* source, size_t bufferSize) override;
	void CreateBuffers(ID3D12Device* device, bool msaa = false) override;
	void CopyData() noexcept override;
	void RecordUpload(ID3D12GraphicsCommandList* copyList) override;
	void ReleaseUploadBuffer() override;

private:
	struct BufferData {
		std::unique_ptr<IUploadBuffer> buffer;
		const void* data;
		size_t size;
		size_t offset;
		BufferType type;
	};

private:
	void CreatePlacedResource(
		ID3D12Device* device, ID3D12Heap* memory, ID3D12Resource* resource,
		size_t offset, const D3D12_RESOURCE_DESC& desc, bool upload
	) const;

	D3D12_RESOURCE_DESC GetBufferDesc(size_t bufferSize) const noexcept;
	D3D12_RESOURCE_DESC GetTextureDesc(size_t height, size_t width) const noexcept;

private:
	size_t m_currentMemoryOffset;
	std::vector<BufferData> m_uploadBufferData;
	std::vector<ComPtr<ID3D12Resource>> m_gpuBuffers;
	std::unique_ptr<ID3DHeap> m_uploadHeap;
	std::unique_ptr<ID3DHeap> m_gpuHeap;
};
#endif
