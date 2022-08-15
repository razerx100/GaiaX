#ifndef TEXTURE_STORAGE_HPP_
#define TEXTURE_STORAGE_HPP_
#include <D3DHeaders.hpp>
#include <DescriptorTableManager.hpp>
#include <vector>
#include <GaiaDataTypes.hpp>
#include <D3DResource.hpp>

class TextureStorage {
public:
	TextureStorage() noexcept;

	size_t AddTexture(
		ID3D12Device* device,
		std::unique_ptr<std::uint8_t> textureDataHandle,
		size_t width, size_t height, bool components16bits
	) noexcept;
	[[nodiscard]]

	void CopyData(std::atomic_size_t& workCount) noexcept;
	void ReleaseUploadBuffer() noexcept;
	void CreateBufferViews(ID3D12Device* device);

	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureDescriptorStart() const noexcept;

private:
	struct TextureData {
		DXGI_FORMAT textureFormat;
		size_t rowPitch;
		size_t height;
	};

private:
	std::vector<std::unique_ptr<std::uint8_t>> m_dataHandles;
	std::vector<TextureData> m_textureData;

	std::vector<D3DResourceShared> m_gpuBuffers;
	std::vector<D3DResourceShared> m_uploadBuffers;
	std::vector<size_t> m_textureDescriptorRelativeOffsets;
	D3D12_GPU_DESCRIPTOR_HANDLE m_textureDescriptorStart;
};
#endif
