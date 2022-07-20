#ifndef HEAP_MANAGER_HPP_
#define HEAP_MANAGER_HPP_
#include <D3DHeap.hpp>
#include <D3DBuffer.hpp>
#include <UploadBuffer.hpp>
#include <vector>
#include <memory>

using SharedBufferPair = std::pair<D3DBufferShared, UploadBufferShared>;

class HeapManager {
public:
	HeapManager();

	[[nodiscard]]
	SharedBufferPair AddUploadAbleBuffer(size_t bufferSize, bool uav = false);
	[[nodiscard]]
	SharedBufferPair AddTexture(
		ID3D12Device* device,
		size_t width, size_t height,
		size_t pixelSizeInBytes,
		bool uav = false,
		bool msaa = false
	);
	[[nodiscard]]
	D3DBufferShared AddBufferGPUOnly(size_t bufferSize, bool uav = false);

	void CreateBuffers(ID3D12Device* device, bool msaa = false);
	void RecordUpload(ID3D12GraphicsCommandList* copyList);
	void ReleaseUploadBuffer();

private:
	struct BufferData {
		bool isTexture;
		size_t width;
		size_t height;
		size_t alignment;
		size_t offset;
		size_t rowPitch;
		DXGI_FORMAT textureFormat;
		bool isUAV;
	};

private:
	void CreatePlacedResource(
		ID3D12Device* device, ID3D12Heap* memory, D3DBufferShared resource,
		size_t offset, const D3D12_RESOURCE_DESC& desc, bool upload
	) const;

	[[nodiscard]]
	D3D12_RESOURCE_DESC	GetResourceDesc(
		const BufferData& bufferData, bool texture
	) const noexcept;
	[[nodiscard]]
	D3D12_RESOURCE_DESC GetBufferDesc(size_t bufferSize, bool uav) const noexcept;
	[[nodiscard]]
	D3D12_RESOURCE_DESC GetTextureDesc(
		size_t height, size_t width, size_t alignment, DXGI_FORMAT textureFormat
	) const noexcept;

	void PopulateAliasingBarrier(
		D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* buffer
	) const noexcept;

private:
	size_t m_currentMemoryOffset;
	std::vector<BufferData> m_bufferData;
	std::vector<BufferData> m_bufferDataGPUOnly;
	std::vector<UploadBufferShared> m_uploadBuffers;
	std::vector<D3DBufferShared> m_gpuBuffers;
	std::vector<D3DBufferShared> m_gpuOnlyBuffers;
	std::unique_ptr<D3DHeap> m_uploadHeap;
	std::unique_ptr<D3DHeap> m_gpuHeap;
	size_t m_maxAlignment;
};
#endif
