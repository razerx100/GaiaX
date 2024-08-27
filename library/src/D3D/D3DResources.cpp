#include <D3DResources.hpp>
#include <Gaia.hpp>
#include <cassert>
#include <Exception.hpp>

// Resource
Resource::Resource(ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType)
	: m_resource{ nullptr }, m_device{ device }, m_memoryManager{ memoryManager },
	m_allocationInfo{
		.heapOffset = 0u,
		.heap       = nullptr,
		.size       = 0u,
		.alignment  = 0u,
		.memoryID   = 0u,
		.isValid    = false
	},
	m_resourceType{ memoryType }
{}

void Resource::Allocate(const D3D12_RESOURCE_DESC& resourceDesc, bool msaa)
{
	if (m_memoryManager)
		m_allocationInfo = m_memoryManager->Allocate(resourceDesc, m_resourceType, msaa);
	else
		throw Exception("MemoryManager Exception", "Memory manager unavailable.");
}

void Resource::Deallocate(bool msaa) noexcept
{
	if (m_memoryManager && m_allocationInfo.isValid)
	{
		m_memoryManager->Deallocate(m_allocationInfo, m_resourceType, msaa);

		m_allocationInfo.isValid = false;
	}
}

void Resource::CreatePlacedResource(
	const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState,
	const D3D12_CLEAR_VALUE* clearValue
) {
	m_device->CreatePlacedResource(
		m_allocationInfo.heap, m_allocationInfo.heapOffset,
		&resourceDesc, initialState, clearValue, IID_PPV_ARGS(&m_resource)
	);
}

// Buffer
Buffer::Buffer(ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType)
	: Resource{ device, memoryManager, memoryType }, m_cpuHandle{ nullptr }, m_bufferSize{ 0u }
{}

Buffer::~Buffer() noexcept
{
	SelfDestruct();
}

void Buffer::Create(UINT64 bufferSize, D3D12_RESOURCE_STATES initialState)
{
	D3D12_RESOURCE_DESC bufferDesc
	{
		.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment        = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		.Width            = bufferSize,
		.Height           = 1u,
		.DepthOrArraySize = 1u,
		.MipLevels        = 1u,
		.Format           = DXGI_FORMAT_UNKNOWN,
		.SampleDesc       = DXGI_SAMPLE_DESC{ .Count = 1u, .Quality = 0u },
		.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags            = D3D12_RESOURCE_FLAG_NONE
	};

	// If the buffer pointer is already allocated, then free it.
	if (m_resource != nullptr)
		Destroy();

	Allocate(bufferDesc, false);

	CreatePlacedResource(bufferDesc, initialState, nullptr);

	// If the buffer is of the type Upload, map the buffer to the cpu handle.
	if (GetHeapType() == D3D12_HEAP_TYPE_UPLOAD)
		m_resource->Map(0u, nullptr, reinterpret_cast<void**>(&m_cpuHandle));

	m_bufferSize = bufferSize;
}

void Buffer::Destroy() noexcept
{
	SelfDestruct();
	m_resource.Reset();
}

void Buffer::SelfDestruct() noexcept
{
	Deallocate(false);
}

// Texture
Texture::Texture(ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType)
	: Resource{ device, memoryManager, memoryType }, m_format{ DXGI_FORMAT_UNKNOWN },
	m_width{ 0u }, m_height{ 0u }, m_depth{ 0u }, m_msaa{ false }
{}

Texture::~Texture() noexcept
{
	SelfDestruct();
}

void Texture::Create(
	UINT64 width, UINT height, UINT16 depth, UINT16 mipLevels, DXGI_FORMAT textureFormat,
	D3D12_RESOURCE_DIMENSION resourceDimension, D3D12_RESOURCE_STATES initialState,
	const D3D12_CLEAR_VALUE* clearValue, bool msaa
) {
	m_width  = width;
	m_height = height;
	m_depth  = depth;

	m_format = textureFormat;
	m_msaa   = msaa;

	const UINT64 textureAlignment = msaa ?
		D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

	D3D12_RESOURCE_DESC textureDesc
	{
		.Dimension        = resourceDimension,
		.Alignment        = textureAlignment,
		.Width            = width,
		.Height           = height,
		.DepthOrArraySize = depth,
		.MipLevels        = mipLevels,
		.Format           = textureFormat,
		.SampleDesc       = DXGI_SAMPLE_DESC{ .Count = 1u, .Quality = 0u },
		.Layout           =	D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags            = D3D12_RESOURCE_FLAG_NONE
	};

	// If the texture pointer is already allocated, then free it.
	if (m_resource != nullptr)
		Destroy();

	Allocate(textureDesc, msaa);

	CreatePlacedResource(textureDesc, initialState, clearValue);
}

void Texture::Create2D(
	UINT64 width, UINT height, UINT16 mipLevels, DXGI_FORMAT textureFormat,
	D3D12_RESOURCE_STATES initialState,
	bool msaa /* = false */ , const D3D12_CLEAR_VALUE* clearValue /* = nullptr */
) {
	Create(
		width, height, 1u, mipLevels, textureFormat, D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		initialState, clearValue, msaa
	);
}

void Texture::Create3D(
	UINT64 width, UINT height, UINT16 depth, UINT16 mipLevels, DXGI_FORMAT textureFormat,
	D3D12_RESOURCE_STATES initialState,
	bool msaa /* = false */ , const D3D12_CLEAR_VALUE* clearValue /* = nullptr */
) {
	Create(
		width, height, depth, mipLevels, textureFormat, D3D12_RESOURCE_DIMENSION_TEXTURE3D,
		initialState, clearValue, msaa
	);
}

void Texture::Destroy() noexcept
{
	SelfDestruct();
	m_resource.Reset();
}

void Texture::SelfDestruct() noexcept
{
	Deallocate(m_msaa);
}

UINT64 Texture::GetBufferSize() const noexcept
{
	// For example: R8G8B8A8 has 4 components, 8bits at each component. So, 4bytes.
	const static std::unordered_map<DXGI_FORMAT, std::uint32_t> formatSizeMap
	{
		{ DXGI_FORMAT_R8G8B8A8_TYPELESS,    4u },
		{ DXGI_FORMAT_R8G8B8A8_UNORM,       4u },
		{ DXGI_FORMAT_R8G8B8A8_SNORM,       4u },
		{ DXGI_FORMAT_R8G8B8A8_UINT,        4u },
		{ DXGI_FORMAT_R8G8B8A8_SINT,        4u },
		{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  4u },
		{ DXGI_FORMAT_B8G8R8A8_TYPELESS,    4u },
		{ DXGI_FORMAT_B8G8R8A8_UNORM,       4u },
		{ DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,  4u }
	};

	const DXGI_FORMAT textureFormat = Format();

	auto formatSize = formatSizeMap.find(textureFormat);

	if (formatSize != std::end(formatSizeMap))
	{
		const UINT64 sizePerPixel = formatSize->second;
		const UINT64 rowPitch     = Align(m_width * sizePerPixel, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

		return rowPitch * m_height * m_depth;
	}

	// Not bothering with adding the size of every single colour format. If an format size isn't
	// defined here, then 0 will be returned.
	return 0u;
}

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
