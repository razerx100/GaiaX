#ifndef TEXTURE_STORAGE_HPP_
#define TEXTURE_STORAGE_HPP_
#include <D3DHeaders.hpp>
#include <DescriptorTableManager.hpp>
#include <D3DBuffer.hpp>
#include <vector>
#include <UploadBuffer.hpp>

class TextureStorage {
public:
	size_t AddTexture(
		ID3D12Device* device,
		std::unique_ptr<std::uint8_t> textureDataHandle,
		size_t width, size_t height, size_t pixelSizeInBytes
	) noexcept;
	[[nodiscard]]
	size_t GetPhysicalIndexFromVirtual(size_t virtualIndex) const noexcept;

	void CopyData(std::atomic_size_t& workCount) noexcept;
	void ReleaseUploadBuffer() noexcept;
	void CreateBufferViews(ID3D12Device* device);

private:
	struct TextureData {
		size_t textureSize;
		DXGI_FORMAT textureFormat;
	};

private:
	std::vector<std::unique_ptr<std::uint8_t>> m_dataHandles;
	std::vector<TextureData> m_textureData;
	std::vector<SharedIndex> m_textureIndices;

	std::vector<std::shared_ptr<D3DBuffer>> m_gpuBuffers;
	std::vector<std::shared_ptr<UploadBuffer>> m_uploadBuffers;
	std::vector<SharedCPUHandle> m_cpuHandles;
};
#endif
