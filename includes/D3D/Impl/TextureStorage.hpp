#ifndef __TEXTURE_STORAGE_HPP__
#define __TEXTURE_STORAGE_HPP__
#include <ITextureStorage.hpp>
#include <IResourceBuffer.hpp>
#include <IDescriptorTableManager.hpp>
#include <vector>

class TextureStorage : public ITextureStorage {
public:
	size_t AddTexture(const void* data, size_t rowPitch, size_t rows) noexcept override;
	size_t GetPhysicalIndexFromVirtual(size_t virtualIndex) const noexcept override;

	void CreateBufferViews(ID3D12Device* device) override;

private:
	struct Bufferdata {
		SharedCPUHandle handle;
		D3DGPUSharedAddress address;
		size_t bufferSize;
	};

private:
	std::vector<Bufferdata> m_textureData;
	std::vector<SharedIndex> m_textureIndices;
};
#endif
