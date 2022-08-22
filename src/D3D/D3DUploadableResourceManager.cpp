#include <D3DUploadableResourceManager.hpp>

void D3DUploadableResourceManager::CreateResource(ID3D12Device* device) {
	m_resourceView.CreateResource(device);
}

void D3DUploadableResourceManager::RecordResourceUpload(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_resourceView.RecordResourceUpload(copyList);
}

void D3DUploadableResourceManager::ReleaseUploadResource() noexcept {
	m_resourceView.ReleaseUploadResource();
}
