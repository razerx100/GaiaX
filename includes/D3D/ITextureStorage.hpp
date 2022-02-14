#ifndef __I_TEXTURE_STORAGE_HPP__
#define __I_TEXTURE_STORAGE_HPP__
#include <D3DHeaders.hpp>

class ITextureStorage {
public:
	virtual ~ITextureStorage() = default;

	virtual size_t AddColor(const void* data, size_t bufferSize) noexcept = 0;
	virtual size_t GetPhysicalIndexFromVirtual(size_t virtualIndex) const noexcept = 0;

	virtual void CreateBufferViews(ID3D12Device* device) = 0;
};

ITextureStorage* CreateTextureStorageInstance();
#endif
