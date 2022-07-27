#ifndef HEAP_MANAGER_HPP_
#define HEAP_MANAGER_HPP_
#include <vector>
#include <memory>
#include <D3DHeaders.hpp>

import D3DHeap;
import D3DResource;

using SharedBufferPair = std::pair<D3DResourceShared, D3DCPUWResourceShared>;

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
	D3DResourceShared AddBufferGPUOnly(size_t bufferSize, bool uav = false);

	void CreateBuffers(ID3D12Device* device);
	void ReserveHeapSpace() noexcept;
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
	std::vector<D3DCPUWResourceShared> m_uploadBuffers;
	std::vector<D3DResourceShared> m_gpuBuffers;
	std::vector<D3DResourceShared> m_gpuOnlyBuffers;
	size_t m_maxAlignment;
	size_t m_uploadHeapBaseOffset;
	size_t m_gpuReadOnlyHeapBaseOffset;
};
#endif
