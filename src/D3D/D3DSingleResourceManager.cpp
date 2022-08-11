#include <D3DSingleResourceManager.hpp>

D3DSingleResourceManager::D3DSingleResourceManager(
	ResourceType type, D3D12_RESOURCE_FLAGS flags
) : m_resourceView{ type, flags } {}

void D3DSingleResourceManager::CreateResource(ID3D12Device* device) {
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	if (auto type = m_resourceView.GetResourceType();
		type == ResourceType::cpuReadBack || type == ResourceType::gpuOnly)
		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
	else
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

	m_resourceView.CreateResource(device, initialState, nullptr);
}

void D3DSingleResourceManager::ReserveHeapSpace(ID3D12Device* device) {
	// 256 bytes alignment is only required if the first bufferView is a constant buffer. But
	// it wouldn't cause problem with other types of resources either as it's also 4 bytes
	// aligned. And as it's only the starting address there won't be much memory wastage either
	// if the first bufferView isn't even a Constant Buffer.
	constexpr size_t bufferAlignment = 256u;

	m_resourceView.SetBufferInfo(bufferAlignment, m_linearAllocator.GetTotalSize());
	m_resourceView.ReserveHeapSpace(device);
}

size_t D3DSingleResourceManager::ReserveSpaceAndGetOffset(
	size_t size, size_t alignment
) noexcept {
	return m_linearAllocator.SubAllocate(size, alignment);
}

D3D12_GPU_VIRTUAL_ADDRESS D3DSingleResourceManager::GetGPUStartAddress() const noexcept {
	return m_resourceView.GetGPUAddress();
}

std::uint8_t* D3DSingleResourceManager::GetCPUStartAddress() const {
	return m_resourceView.GetCPUWPointer();
}
