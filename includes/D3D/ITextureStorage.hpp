#ifndef __I_TEXTURE_STORAGE_HPP__
#define __I_TEXTURE_STORAGE_HPP__
#include <D3DHeaders.hpp>
#include <atomic>

class ITextureStorage {
public:
	virtual ~ITextureStorage() = default;

	virtual size_t AddTexture(
		ID3D12Device* device,
		const void* data, size_t rowPitch, size_t rows
	) noexcept = 0;
	virtual size_t GetPhysicalIndexFromVirtual(size_t virtualIndex) const noexcept = 0;

	virtual void CopyData(std::atomic_size_t& workCount) noexcept = 0;
	virtual void ReleaseUploadBuffer() noexcept = 0;
	virtual void CreateBufferViews(ID3D12Device* device) = 0;
};

ITextureStorage* CreateTextureStorageInstance();
#endif
