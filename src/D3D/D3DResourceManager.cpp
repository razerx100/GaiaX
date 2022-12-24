#include <D3DResourceManager.hpp>

D3DResourceManager::D3DResourceManager(
	ResourceType type, D3D12_RESOURCE_FLAGS flags
) : _D3DResourceManager<D3DResourceView>{ type, flags } {}

void D3DResourceManager::CreateResource(ID3D12Device* device) {
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	if (auto type = m_resourceView.GetResourceType();
		type == ResourceType::cpuReadBack || type == ResourceType::gpuOnly)
		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
	else
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

	m_resourceView.CreateResource(device, initialState);
}

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

