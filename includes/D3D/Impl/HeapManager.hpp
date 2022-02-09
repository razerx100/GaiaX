#ifndef __HEAP_MANAGER_HPP__
#define __HEAP_MANAGER_HPP__
#include <IHeapManager.hpp>
#include <ID3DHeap.hpp>
#include <vector>

class HeapManager : public IHeapManager {
public:
	HeapManager();

	BufferPair AddBuffer(
		size_t bufferSize, BufferType type
	) override;
	BufferPair AddTexture(size_t bufferSize) override;
	void CreateBuffers(ID3D12Device* device, bool msaa = false) override;
	void RecordUpload(ID3D12GraphicsCommandList* copyList) override;
	void ReleaseUploadBuffer() override;

private:
	struct BufferData {
		size_t size;
		size_t offset;
		BufferType type;
	};

private:
	void CreatePlacedResource(
		ID3D12Device* device, ID3D12Heap* memory, std::shared_ptr<D3DBuffer> resource,
		size_t offset, const D3D12_RESOURCE_DESC& desc, bool upload
	) const;

	D3D12_RESOURCE_DESC GetBufferDesc(size_t bufferSize) const noexcept;
	D3D12_RESOURCE_DESC GetTextureDesc(size_t height, size_t width) const noexcept;

private:
	size_t m_currentMemoryOffset;
	std::vector<BufferData> m_bufferData;
	std::vector<std::shared_ptr<IUploadBuffer>> m_uploadBuffers;
	std::vector<std::shared_ptr<D3DBuffer>> m_gpuBuffers;
	std::unique_ptr<ID3DHeap> m_uploadHeap;
	std::unique_ptr<ID3DHeap> m_gpuHeap;
};
#endif
