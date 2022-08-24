#include <D3DDescriptorView.hpp>
#include <Gaia.hpp>

// D3D Root Descriptor View
void D3DRootDescriptorView::SetAddressesStart(
	size_t addressesStart, size_t subAllocationSize, size_t alignment
) noexcept {
	m_cpuAddress.SetAddress(addressesStart, subAllocationSize, alignment);
	m_gpuAddress.SetAddress(addressesStart, subAllocationSize, alignment);
}

void D3DRootDescriptorView::UpdateCPUAddressStart(std::uint8_t* offset) noexcept {
	m_cpuAddress.UpdateAddressStart(offset);
}

void D3DRootDescriptorView::UpdateGPUAddressStart(D3D12_GPU_VIRTUAL_ADDRESS offset) noexcept {
	m_gpuAddress.UpdateAddressStart(offset);
}

std::uint8_t* D3DRootDescriptorView::GetCPUAddressStart(size_t index) const noexcept {
	return m_cpuAddress.GetAddressPTRStart<std::uint8_t>(index);
}

D3D12_GPU_VIRTUAL_ADDRESS D3DRootDescriptorView::GetGPUAddressStart(
	size_t index
) const noexcept {
	return m_gpuAddress.GetAddressStart<D3D12_GPU_VIRTUAL_ADDRESS>(index);
}

// D3D Upload Resource Descriptor View
void D3DUploadResourceDescriptorView::RecordResourceUpload(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_resourceBuffer.RecordResourceUpload(copyList);
}

void D3DUploadResourceDescriptorView::ReleaseUploadResource() noexcept {
	m_resourceBuffer.ReleaseUploadResource();
}

D3D12_GPU_VIRTUAL_ADDRESS D3DUploadResourceDescriptorView::GetGPUAddress() const noexcept {
	return m_resourceBuffer.GetGPUAddress();
}

D3D12_RESOURCE_DESC D3DUploadResourceDescriptorView::GetUploadResourceDesc() const noexcept {
	return m_resourceBuffer.GetUploadResourceDesc();
}
