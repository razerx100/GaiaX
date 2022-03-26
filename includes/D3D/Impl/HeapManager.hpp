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
	BufferPair AddTexture(
		ID3D12Device* device,
		size_t width, size_t height,
		size_t pixelSizeInBytes,
		bool msaa = false
	) override;
	void CreateBuffers(ID3D12Device* device, bool msaa = false) override;
	void RecordUpload(ID3D12GraphicsCommandList* copyList) override;
	void ReleaseUploadBuffer() override;

private:
	struct BufferData {
		BufferType type;
		size_t width;
		size_t height;
		size_t alignment;
		size_t offset;
		size_t rowPitch;
		size_t bufferSize;
		DXGI_FORMAT textureFormat;
	};

private:
	void CreatePlacedResource(
		ID3D12Device* device, ID3D12Heap* memory, std::shared_ptr<D3DBuffer> resource,
		size_t offset, const D3D12_RESOURCE_DESC& desc, bool upload
	) const;

	D3D12_RESOURCE_DESC	GetResourceDesc(
		const BufferData& bufferData, bool texture
	) const noexcept;
	D3D12_RESOURCE_DESC GetBufferDesc(size_t bufferSize) const noexcept;
	D3D12_RESOURCE_DESC GetTextureDesc(
		size_t height, size_t width, size_t alignment,
		DXGI_FORMAT textureFormat
	) const noexcept;

private:
	size_t m_currentMemoryOffset;
	std::vector<BufferData> m_bufferData;
	std::vector<std::shared_ptr<IUploadBuffer>> m_uploadBuffers;
	std::vector<std::shared_ptr<D3DBuffer>> m_gpuBuffers;
	std::unique_ptr<ID3DHeap> m_uploadHeap;
	std::unique_ptr<ID3DHeap> m_gpuHeap;
};
#endif
