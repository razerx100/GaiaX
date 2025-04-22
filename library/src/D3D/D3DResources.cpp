#include <D3DResources.hpp>
#include <unordered_map>
#include <cassert>
#include <GaiaException.hpp>

// Resource
Resource::Resource()
	: m_resource{ nullptr }, m_device{ nullptr }, m_memoryManager{ nullptr },
	m_allocationInfo{
		.heapOffset = 0u,
		.heap       = nullptr,
		.size       = 0u,
		.alignment  = 0u,
		.memoryID   = 0u,
		.isValid    = false
	},
	m_resourceType{ D3D12_HEAP_TYPE_CUSTOM }
{}

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
Buffer::Buffer() : Resource{}, m_cpuHandle{ nullptr }, m_bufferSize{ 0u } {}

Buffer::Buffer(ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType)
	: Resource{ device, memoryManager, memoryType }, m_cpuHandle{ nullptr }, m_bufferSize{ 0u }
{}

Buffer::~Buffer() noexcept
{
	SelfDestruct();
}

void Buffer::Create(
	UINT64 bufferSize, D3D12_RESOURCE_STATES initialState,
	D3D12_RESOURCE_FLAGS flags /* = D3D12_RESOURCE_FLAG_NONE */
) {
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
		.Flags            = flags
	};

	// If the buffer pointer is already allocated, then free it.
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

D3D12_UNORDERED_ACCESS_VIEW_DESC Buffer::GetUAVDesc(
	UINT elementCount, UINT strideSize, UINT64 firstElement/* = 0u */, bool raw/* = false */
) noexcept {
	return D3D12_UNORDERED_ACCESS_VIEW_DESC
	{
		.Format        = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
		.Buffer        = D3D12_BUFFER_UAV
		{
			.FirstElement        = firstElement,
			.NumElements         = elementCount,
			.StructureByteStride = strideSize,
			.Flags               = raw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE
		}
	};
}

D3D12_SHADER_RESOURCE_VIEW_DESC Buffer::GetSRVDesc(
	UINT elementCount, UINT strideSize, UINT64 firstElement/* = 0u */, bool raw/* = false */
) noexcept {
	return D3D12_SHADER_RESOURCE_VIEW_DESC
	{
		.Format                  = DXGI_FORMAT_UNKNOWN,
		.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Buffer                  = D3D12_BUFFER_SRV
		{
			.FirstElement        = firstElement,
			.NumElements         = elementCount,
			.StructureByteStride = strideSize,
			.Flags               = raw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE
		}
	};
}

D3D12_CONSTANT_BUFFER_VIEW_DESC Buffer::GetCBVDesc(UINT64 startingOffset, UINT bufferSize) const noexcept
{
	return D3D12_CONSTANT_BUFFER_VIEW_DESC
	{
		.BufferLocation = m_resource->GetGPUVirtualAddress() + startingOffset,
		.SizeInBytes    = bufferSize
	};
}

// Texture
Texture::Texture()
	: Resource{}, m_format{ DXGI_FORMAT_UNKNOWN },
	m_height{ 0u }, m_depth{ 0u }, m_mipLevels{ 0u }, m_msaa{ false }, m_width{ 0u }
{}

Texture::Texture(ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType)
	: Resource{ device, memoryManager, memoryType }, m_format{ DXGI_FORMAT_UNKNOWN },
	m_height{ 0u }, m_depth{ 0u }, m_mipLevels{ 0u }, m_msaa{ false }, m_width{ 0u }
{}

Texture::~Texture() noexcept
{
	SelfDestruct();
}

void Texture::Create(
	UINT64 width, UINT height, UINT16 depth, UINT16 mipLevels, DXGI_FORMAT textureFormat,
	D3D12_RESOURCE_DIMENSION resourceDimension, D3D12_RESOURCE_STATES initialState,
	D3D12_RESOURCE_FLAGS flags, const D3D12_CLEAR_VALUE* clearValue, bool msaa
) {
	m_width  = width;
	m_height = height;
	m_depth  = depth;

	m_mipLevels = mipLevels;
	m_format    = textureFormat;
	m_msaa      = msaa;

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
		.Flags            = flags
	};

	// If the texture pointer is already allocated, then free it.
	Destroy();

	Allocate(textureDesc, msaa);

	CreatePlacedResource(textureDesc, initialState, clearValue);
}

void Texture::Create2D(
	UINT64 width, UINT height, UINT16 mipLevels, DXGI_FORMAT textureFormat,
	D3D12_RESOURCE_STATES initialState,
	D3D12_RESOURCE_FLAGS flags /* = D3D12_RESOURCE_FLAG_NONE */,
	bool msaa /* = false */ , const D3D12_CLEAR_VALUE* clearValue /* = nullptr */
) {
	Create(
		width, height, 1u, mipLevels, textureFormat, D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		initialState, flags, clearValue, msaa
	);
}

void Texture::Create3D(
	UINT64 width, UINT height, UINT16 depth, UINT16 mipLevels, DXGI_FORMAT textureFormat,
	D3D12_RESOURCE_STATES initialState,
	D3D12_RESOURCE_FLAGS flags /* = D3D12_RESOURCE_FLAG_NONE */,
	bool msaa /* = false */ , const D3D12_CLEAR_VALUE* clearValue /* = nullptr */
) {
	Create(
		width, height, depth, mipLevels, textureFormat, D3D12_RESOURCE_DIMENSION_TEXTURE3D,
		initialState, flags, clearValue, msaa
	);
}

void Texture::Recreate2D(
	D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_FLAGS flags,
	const D3D12_CLEAR_VALUE* clearValue
) {
	Create2D(
		m_width, m_height, m_mipLevels, m_format, currentState, flags, m_msaa, clearValue
	);
}

void Texture::Recreate3D(
	D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_FLAGS flags,
	const D3D12_CLEAR_VALUE* clearValue
) {
	Create3D(
		m_width, m_height, m_depth, m_mipLevels, m_format, currentState, flags, m_msaa, clearValue
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

D3D12_SHADER_RESOURCE_VIEW_DESC Texture::GetSRVDesc(
	UINT mostDetailedMip /* = 0u */, UINT mipLevels /* = 1u */
) const noexcept {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
	{
		.Format                  = m_format,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
	};

	if (m_depth > 1u)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D,
		srvDesc.Texture3D     = D3D12_TEX3D_SRV
		{
			.MostDetailedMip = mostDetailedMip,
			.MipLevels       = mipLevels
		};
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
		srvDesc.Texture2D     = D3D12_TEX2D_SRV
		{
			.MostDetailedMip = mostDetailedMip,
			.MipLevels       = mipLevels
		};
	}

	return srvDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC Texture::GetUAVDesc(UINT mipSlice/* = 0u */) const noexcept
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc
	{
		.Format = m_format
	};

	if (m_depth > 1u)
	{
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D,
		uavDesc.Texture3D     = D3D12_TEX3D_UAV
		{
			.MipSlice = mipSlice
		};
	}
	else
	{
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
		uavDesc.Texture2D     = D3D12_TEX2D_UAV
		{
			.MipSlice = mipSlice
		};
	}

	return uavDesc;
}

UINT64 Texture::GetRowPitch() const noexcept
{
	// For example: R8G8B8A8 has 4 components, 8bits at each component. So, 4bytes.
	const static std::unordered_map<DXGI_FORMAT, UINT> formatSizeMap
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

	// Not bothering with adding the size of every single colour format. If a format size isn't
	// defined here, then 0 will be returned.
	UINT64 rowPitch                 = 0u;

	auto formatSize = formatSizeMap.find(textureFormat);

	assert(formatSize != std::end(formatSizeMap) && "Texture format isn't available in the FormatSizeMap.");

	if (formatSize != std::end(formatSizeMap))
	{
		const UINT64 sizePerPixel = formatSize->second;
		rowPitch                  = m_width * sizePerPixel;
	}

	return rowPitch;
}

UINT64 Texture::GetRowPitchD3DAligned() const noexcept
{
	return Callisto::Align(GetRowPitch(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
}
