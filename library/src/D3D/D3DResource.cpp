#include <D3DResource.hpp>
#include <D3DHelperFunctions.hpp>
#include <Gaia.hpp>
#include <cassert>

// D3DResource
D3DResource::D3DResource() noexcept : m_cpuHandle{ nullptr } {}

ID3D12Resource* D3DResource::Get() const noexcept {
	return m_pBuffer.Get();
}

ID3D12Resource** D3DResource::GetAddress() noexcept {
	return m_pBuffer.GetAddressOf();
}

ID3D12Resource** D3DResource::ReleaseAndGetAddress() noexcept {
	return m_pBuffer.ReleaseAndGetAddressOf();
}

void D3DResource::CreateResource(
	ID3D12Device* device, ID3D12Heap* heap, size_t offset, const D3D12_RESOURCE_DESC& desc,
	D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue
) {
	device->CreatePlacedResource(
		heap, offset, &desc, initialState, clearValue, IID_PPV_ARGS(&m_pBuffer)
	);
}

void D3DResource::Release() noexcept {
	m_pBuffer.Reset();
}

void D3DResource::MapBuffer() {
	D3D12_RANGE const* rangePtr = nullptr;

	m_pBuffer->Map(0u, rangePtr, reinterpret_cast<void**>(&m_cpuHandle));
}

std::uint8_t* D3DResource::GetCPUWPointer() const noexcept {
	return m_cpuHandle;
}

// D3DResourceView
D3DResourceView::D3DResourceView(ResourceType type, D3D12_RESOURCE_FLAGS flags) noexcept
	: m_resourceDescription{}, m_heapOffset{ 0u }, m_type{ type }, m_subAllocationSize{ 0u },
	m_subAllocationCount{ 0u }, m_subBufferSize{ 0u } {

	m_resourceDescription.DepthOrArraySize = 1u;
	m_resourceDescription.SampleDesc.Count = 1u;
	m_resourceDescription.SampleDesc.Quality = 0u;
	m_resourceDescription.Flags = flags;
}

void D3DResourceView::_setBufferInfo(
	UINT64 bufferSize, D3D12_RESOURCE_DESC& resourceDesc
) noexcept {
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Width = bufferSize;
	resourceDesc.Height = 1u;
	resourceDesc.MipLevels = 1u;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
}

void D3DResourceView::SetBufferInfo(
	UINT64 bufferSize, UINT64 allocationCount, UINT64 alignment
) noexcept {
	m_subAllocationSize = Align(bufferSize, alignment);
	m_subBufferSize = bufferSize;
	m_subAllocationCount = allocationCount;
	bufferSize = m_subAllocationSize * (m_subAllocationCount - 1u) + m_subBufferSize;

	_setBufferInfo(bufferSize, m_resourceDescription);
}

UINT64 D3DResourceView::CalculateRowPitch(UINT64 width) noexcept {
	size_t rowPitch = width * 4u;

	return Align(rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
}

void D3DResourceView::_setTextureInfo(
	UINT64 width, UINT height, DXGI_FORMAT format, bool msaa,
	D3D12_RESOURCE_DESC& resourceDesc
) noexcept {
	size_t estimatedSize = static_cast<size_t>(CalculateRowPitch(width)) * height;
	size_t alignment = 0u;

	if (msaa) {
		if (estimatedSize <= D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT)
			alignment = D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		else
			alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
	}
	else {
		if (estimatedSize <= D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
			alignment = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
		else
			alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	}

	resourceDesc.Format = format;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1u;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Alignment = alignment;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

void D3DResourceView::SetTextureInfo(
	UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
) noexcept {
	_setTextureInfo(width, height, format, msaa, m_resourceDescription);
}

void D3DResourceView::ReserveHeapSpace(ID3D12Device* device) noexcept {
	const auto& [bufferSize, alignment] = device->GetResourceAllocationInfo(
		0u, 1u, &m_resourceDescription
	);

	/*
	if (m_type == ResourceType::gpuOnly)
		m_heapOffset = Gaia::Resources::gpuOnlyHeap->ReserveSizeAndGetOffset(
			bufferSize, alignment
		);
	else if(m_type == ResourceType::upload)
		m_heapOffset = Gaia::Resources::uploadHeap->ReserveSizeAndGetOffset(
			bufferSize, alignment
		);
	else if(m_type == ResourceType::cpuWrite)
		m_heapOffset = Gaia::Resources::cpuWriteHeap->ReserveSizeAndGetOffset(
			bufferSize, alignment
		);
	else if(m_type == ResourceType::cpuReadBack)
		m_heapOffset = Gaia::Resources::cpuReadBackHeap->ReserveSizeAndGetOffset(
			bufferSize, alignment
		);
	*/
}

void D3DResourceView::CreateResource(
	ID3D12Device* device, D3D12_RESOURCE_STATES initialState,
	const D3D12_CLEAR_VALUE* clearValue
) {
	ID3D12Heap* pHeap = nullptr;

	if (m_type == ResourceType::cpuWrite)
		pHeap = Gaia::Resources::cpuWriteHeap->Get();
	else if (m_type == ResourceType::upload)
		pHeap = Gaia::Resources::uploadHeap->Get();
	else if (m_type == ResourceType::gpuOnly)
		pHeap = Gaia::Resources::gpuOnlyHeap->Get();
	else if (m_type == ResourceType::cpuReadBack)
		pHeap = Gaia::Resources::cpuReadBackHeap->Get();

	m_resource.CreateResource(
		device, pHeap, m_heapOffset, m_resourceDescription, initialState, clearValue
	);

	if (m_type != ResourceType::gpuOnly)
		m_resource.MapBuffer();
}

ID3D12Resource* D3DResourceView::GetResource() const noexcept {
	return m_resource.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS D3DResourceView::GetGPUAddress(UINT64 index) const noexcept {
	return m_resource.Get()->GetGPUVirtualAddress() + GetSubAllocationOffset(index);
}

D3D12_GPU_VIRTUAL_ADDRESS D3DResourceView::GetFirstGPUAddress() const noexcept {
	return GetGPUAddress(0u);
}

std::uint8_t* D3DResourceView::GetCPUWPointer(UINT64 index) const noexcept {
	assert(m_type != ResourceType::gpuOnly && "This buffer doesn't have CPU access.");

	return m_resource.GetCPUWPointer() + GetSubAllocationOffset(index);
}

std::uint8_t* D3DResourceView::GetFirstCPUWPointer() const noexcept {
	return GetCPUWPointer(0u);
}

ResourceType D3DResourceView::GetResourceType() const noexcept {
	return m_type;
}

UINT64 D3DResourceView::QueryTextureBufferSize(
	ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
) noexcept {
	D3D12_RESOURCE_DESC	resourceDesc{};
	resourceDesc.DepthOrArraySize = 1u;
	resourceDesc.SampleDesc.Count = 1u;
	resourceDesc.SampleDesc.Quality = 0u;

	_setTextureInfo(width, height, format, msaa, resourceDesc);

	const auto& [bufferSize, alignment] = device->GetResourceAllocationInfo(
		0u, 1u, &resourceDesc
	);

	return bufferSize;
}

void D3DResourceView::ReleaseResource() noexcept {
	m_resource.Release();
}

D3D12_RESOURCE_DESC D3DResourceView::GetResourceDesc() const noexcept {
	return m_resourceDescription;
}

UINT64 D3DResourceView::GetSubAllocationOffset(UINT64 index) const noexcept {
	return m_subAllocationSize * index;
}

UINT64 D3DResourceView::GetFirstSubAllocationOffset() const noexcept {
	return GetSubAllocationOffset(0u);
}

// D3D Upload Resource View
D3DUploadableResourceView::D3DUploadableResourceView(
	ResourceType type, D3D12_RESOURCE_FLAGS flags
) noexcept : m_uploadResource{ ResourceType::upload }, m_gpuResource{ type, flags },
	m_texture{ false } {}

void D3DUploadableResourceView::SetBufferInfo(
	UINT64 bufferSize, UINT64 allocationCount, UINT64 alignment
) noexcept {
	m_uploadResource.SetBufferInfo(bufferSize, allocationCount, alignment);
	m_gpuResource.SetBufferInfo(bufferSize, allocationCount, alignment);
}

void D3DUploadableResourceView::SetTextureInfo(
	ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
) noexcept {
	const UINT64 textureBufferSize = D3DResourceView::QueryTextureBufferSize(
		device, width, height, format, msaa
	);

	m_uploadResource.SetBufferInfo(textureBufferSize, 1u);
	m_gpuResource.SetTextureInfo(width, height, format, msaa);

	m_texture = true;
}

void D3DUploadableResourceView::ReserveHeapSpace(ID3D12Device* device) noexcept {
	m_uploadResource.ReserveHeapSpace(device);
	m_gpuResource.ReserveHeapSpace(device);
}

void D3DUploadableResourceView::CreateResource(
	ID3D12Device* device, D3D12_RESOURCE_STATES initialState
) {
	m_uploadResource.CreateResource(device, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_gpuResource.CreateResource(device, initialState);
}

void D3DUploadableResourceView::RecordResourceUpload(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	if (m_texture) {
		D3D12_TEXTURE_COPY_LOCATION dest = {};
		dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dest.pResource = m_gpuResource.GetResource();
		dest.SubresourceIndex = 0u;

		D3D12_RESOURCE_DESC gpuDesc = m_gpuResource.GetResourceDesc();

		D3D12_SUBRESOURCE_FOOTPRINT srcFootprint = {};
		srcFootprint.Depth = 1u;
		srcFootprint.Format = gpuDesc.Format;
		srcFootprint.Width = static_cast<UINT>(gpuDesc.Width);
		srcFootprint.Height = gpuDesc.Height;
		srcFootprint.RowPitch = static_cast<UINT>(D3DResourceView::CalculateRowPitch(
			gpuDesc.Width
		));

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint = {};
		placedFootprint.Offset = 0u;
		placedFootprint.Footprint = srcFootprint;

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.pResource = m_uploadResource.GetResource();
		src.PlacedFootprint = placedFootprint;

		copyList->CopyTextureRegion(
			&dest,
			0u, 0u, 0u,
			&src,
			nullptr
		);
	}
	else
		copyList->CopyResource(
			m_gpuResource.GetResource(),
			m_uploadResource.GetResource()
		);
}

void D3DUploadableResourceView::ReleaseUploadResource() noexcept {
	m_uploadResource.ReleaseResource();
}

std::uint8_t* D3DUploadableResourceView::GetCPUWPointer(UINT64 index) const noexcept {
	return m_uploadResource.GetCPUWPointer(index);
}

ID3D12Resource* D3DUploadableResourceView::GetResource() const noexcept {
	return m_gpuResource.GetResource();
}

D3D12_GPU_VIRTUAL_ADDRESS D3DUploadableResourceView::GetGPUAddress(
	UINT64 index
) const noexcept {
	return m_gpuResource.GetGPUAddress(index);
}

D3D12_RESOURCE_DESC D3DUploadableResourceView::GetResourceDesc() const noexcept {
	return m_gpuResource.GetResourceDesc();
}

D3D12_GPU_VIRTUAL_ADDRESS D3DUploadableResourceView::GetFirstGPUAddress() const noexcept {
	return m_gpuResource.GetFirstGPUAddress();
}

std::uint8_t* D3DUploadableResourceView::GetFirstCPUWPointer() const noexcept {
	return m_uploadResource.GetFirstCPUWPointer();
}

UINT64 D3DUploadableResourceView::GetSubAllocationOffset(UINT64 index) const noexcept {
	return m_gpuResource.GetSubAllocationOffset(index);
}

UINT64 D3DUploadableResourceView::GetFirstSubAllocationOffset() const noexcept {
	return m_gpuResource.GetFirstSubAllocationOffset();
}
