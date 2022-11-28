#include <D3DSingleResourceManager.hpp>

D3DSingleResourceManager::D3DSingleResourceManager(
	ResourceType type, D3D12_RESOURCE_FLAGS flags
) : _D3DResourceManager<D3DResourceView>{ type, flags } {}

void D3DSingleResourceManager::CreateResource(ID3D12Device* device) {
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	if (auto type = m_resourceView.GetResourceType();
		type == ResourceType::cpuReadBack || type == ResourceType::gpuOnly)
		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
	else
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

	m_resourceView.CreateResource(device, initialState);
}
