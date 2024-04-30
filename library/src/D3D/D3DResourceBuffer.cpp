#include <D3DResourceBuffer.hpp>

// Resource Buffer
D3DResourceBuffer::D3DResourceBuffer(ResourceType type, D3D12_RESOURCE_FLAGS flags)
	: _D3DResourceBuffer<D3DResourceView>{ type, flags } {}

void D3DResourceBuffer::CreateResource(ID3D12Device* device) {
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	if (auto type = m_resourceView.GetResourceType();
		type == ResourceType::cpuReadBack || type == ResourceType::gpuOnly)
		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
	else
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

	m_resourceView.CreateResource(device, initialState);
}

// Resource Buffer Uploadable
void D3DUploadableResourceBuffer::CreateResource(ID3D12Device* device) {
	m_resourceView.CreateResource(device);
}

void D3DUploadableResourceBuffer::RecordResourceUpload(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_resourceView.RecordResourceUpload(copyList);
}

void D3DUploadableResourceBuffer::ReleaseUploadResource() noexcept {
	m_resourceView.ReleaseUploadResource();
}
