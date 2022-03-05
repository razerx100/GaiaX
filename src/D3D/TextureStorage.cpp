#include <TextureStorage.hpp>
#include <InstanceManager.hpp>
#include <CRSMath.hpp>

size_t TextureStorage::AddTexture(const void* data, size_t rowPitch, size_t rows) noexcept {

	return 0u;
}

void TextureStorage::CreateBufferViews(ID3D12Device* device) {

}

size_t TextureStorage::GetPhysicalIndexFromVirtual(size_t virtualIndex) const noexcept {
	return *m_textureIndices[virtualIndex];
}
