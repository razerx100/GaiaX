#include <D3DDescriptorView.hpp>
#include <Gaia.hpp>
#include <D3DHelperFunctions.hpp>

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

void _D3DDescriptorView<D3DUploadableResourceView>::SetTextureInfo(
	ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
) noexcept {
	m_resourceBuffer.SetTextureInfo(device, width, height, format, msaa);
	m_resourceBuffer.ReserveHeapSpace(device);

	m_texture = true;
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

// D3D Descriptor View UAV Counter
D3DDescriptorViewUAVCounter::D3DDescriptorViewUAVCounter(ResourceType type) noexcept
	: _D3DDescriptorViewBase<D3DResourceView>(type, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
	m_counterOffset{ 0u }, m_strideSize{ 0u }, m_elementCount{ 0u } {}

void D3DDescriptorViewUAVCounter::SetBufferInfo(
	ID3D12Device* device, UINT64 strideSize, UINT elementCount
) noexcept {
	m_strideSize = static_cast<UINT>(strideSize);
	m_elementCount = elementCount;

	UINT64 bufferSize = strideSize * elementCount;
	m_counterOffset = Align(bufferSize, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

	SetResourceViewBufferInfo(device, m_counterOffset + sizeof(UINT), m_resourceBuffer);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3DDescriptorViewUAVCounter::GetGPUDescriptorHandle() const noexcept {
	return m_gpuHandleStart;
}

UINT64 D3DDescriptorViewUAVCounter::GetCounterOffset() const noexcept {
	return m_counterOffset;
}

UINT64 D3DDescriptorViewUAVCounter::GetBufferOffset() const noexcept {
	return 0u;
}

void D3DDescriptorViewUAVCounter::CreateBufferDescriptors(ID3D12Device* device) const noexcept {
	D3D12_BUFFER_UAV bufferInfo{
			.FirstElement = 0u,
			.NumElements = m_elementCount,
			.StructureByteStride = m_strideSize,
			.CounterOffsetInBytes = m_counterOffset,
			.Flags = D3D12_BUFFER_UAV_FLAG_NONE
	};

	D3D12_UNORDERED_ACCESS_VIEW_DESC desc{
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
		.Buffer = bufferInfo
	};

	device->CreateUnorderedAccessView(
		m_resourceBuffer.GetResource(), m_resourceBuffer.GetResource(), &desc, m_cpuHandleStart
	);
}

void D3DDescriptorViewUAVCounter::CreateDescriptorView(
	ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart
) noexcept {
	m_cpuHandleStart.ptr = { uploadDescriptorStart.ptr + m_descriptorSize * m_descriptorOffset };
	m_gpuHandleStart.ptr = { gpuDescriptorStart.ptr + m_descriptorSize * m_descriptorOffset };

	m_resourceBuffer.CreateResource(device, D3D12_RESOURCE_STATE_COPY_DEST);
	CreateBufferDescriptors(device);
}

UploadContainer* GetGUploadContainer() noexcept {
	return Gaia::Resources::uploadContainer.get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GetUploadDescsStart() noexcept {
	return Gaia::descriptorTable->GetUploadDescriptorStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescsStart() noexcept {
	return Gaia::descriptorTable->GetGPUDescriptorStart();
}
