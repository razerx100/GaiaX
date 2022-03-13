#ifndef __TEXTURE_STORAGE_HPP__
#define __TEXTURE_STORAGE_HPP__
#include <ITextureStorage.hpp>
#include <IResourceBuffer.hpp>
#include <IDescriptorTableManager.hpp>
#include <vector>
#include <IUploadBuffer.hpp>

class TextureStorage : public ITextureStorage {
public:
	size_t AddTexture(
		ID3D12Device* device,
		const void* data, size_t rowPitch, size_t rows
	) noexcept override;
	size_t GetPhysicalIndexFromVirtual(size_t virtualIndex) const noexcept override;

	void CopyData(std::atomic_size_t& workCount) noexcept override;
	void ReleaseUploadBuffer() noexcept override;
	void CreateBufferViews(ID3D12Device* device) override;

private:
	struct TextureData {
		const void* data;
		size_t textureSize;
	};

private:
	std::vector<TextureData> m_textureData;
	std::vector<SharedIndex> m_textureIndices;

	std::vector<std::shared_ptr<D3DBuffer>> m_gpuBuffers;
	std::vector<std::shared_ptr<IUploadBuffer>> m_uploadBuffers;
	std::vector<SharedCPUHandle> m_cpuHandles;
};
#endif
