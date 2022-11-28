#ifndef TEXTURE_STORAGE_HPP_
#define TEXTURE_STORAGE_HPP_
#include <D3DHeaders.hpp>
#include <DescriptorTableManager.hpp>
#include <vector>
#include <memory>
#include <D3DDescriptorView.hpp>

class TextureStorage {
public:
	TextureStorage() noexcept;

	[[nodiscard]]
	size_t AddTexture(
		ID3D12Device* device, std::unique_ptr<std::uint8_t> textureDataHandle, size_t width,
		size_t height
	) noexcept;

	void SetGraphicsRootSignatureLayout(std::vector<UINT> rsLayout) noexcept;

	void CreateBufferViews(ID3D12Device* device);
	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

	void BindTextures(ID3D12GraphicsCommandList* graphicsList) const noexcept;

private:
	std::vector<std::unique_ptr<D3DUploadResourceDescriptorView>> m_textureDescriptors;
	D3D12_GPU_DESCRIPTOR_HANDLE m_textureDescriptorStart;
	std::vector<std::unique_ptr<std::uint8_t>> m_textureHandles;
	std::vector<UINT> m_graphicsRSLayout;
};
#endif
