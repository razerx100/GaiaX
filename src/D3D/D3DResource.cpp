#include <D3DResource.hpp>
#include <D3DThrowMacros.hpp>
#include <Gaia.hpp>

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
	HRESULT hr{};
	D3D_THROW_FAILED(hr,
		device->CreatePlacedResource(
			heap, offset, &desc, initialState, clearValue, __uuidof(ID3D12Resource), &m_pBuffer
		)
	);
}

void D3DResource::MapBuffer() {
	HRESULT hr{};
	D3D_THROW_FAILED(
		hr,
		m_pBuffer->Map(0u, nullptr, reinterpret_cast<void**>(&m_cpuHandle))
	);
}

std::uint8_t* D3DResource::GetCPUWPointer() const noexcept {
	return m_cpuHandle;
}

// D3DResourceView
D3DResourceView::D3DResourceView(ResourceType type, D3D12_RESOURCE_FLAGS flags) noexcept
	: m_resourceDescription{}, m_heapOffset{ 0u }, m_type{ type } {

	m_resourceDescription.DepthOrArraySize = 1u;
	m_resourceDescription.SampleDesc.Count = 1u;
	m_resourceDescription.SampleDesc.Quality = 0u;
	m_resourceDescription.Flags = flags;
}

void D3DResourceView::SetBufferInfo(UINT64 alignment, UINT64 bufferSize) noexcept {
	m_resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
	m_resourceDescription.Width = bufferSize;
	m_resourceDescription.Height = 1u;
	m_resourceDescription.MipLevels = 1u;
	m_resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	m_resourceDescription.Alignment = alignment;
	m_resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
}

void D3DResourceView::SetTextureInfo(
	UINT64 alignment, UINT64 width, UINT height, DXGI_FORMAT format
) noexcept {
	m_resourceDescription.Format = format;
	m_resourceDescription.Width = width;
	m_resourceDescription.Height = height;
	m_resourceDescription.MipLevels = 0u;
	m_resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	m_resourceDescription.Alignment = alignment;
	m_resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}

void D3DResourceView::ReserveHeapSpace(ID3D12Device* device) noexcept {
	const auto& [bufferSize, alignment] = device->GetResourceAllocationInfo(
		0u, 1u, &m_resourceDescription
	);

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
}

void D3DResourceView::CreateResource(
	ID3D12Device* device, D3D12_RESOURCE_STATES initialState,
	const D3D12_CLEAR_VALUE* clearValue
) {
	ID3D12Heap* pHeap = nullptr;

	if (m_type == ResourceType::cpuWrite)
		pHeap = Gaia::Resources::cpuWriteHeap->GetHeap();
	else if (m_type == ResourceType::upload)
		pHeap = Gaia::Resources::uploadHeap->GetHeap();
	else if (m_type == ResourceType::gpuOnly)
		pHeap = Gaia::Resources::gpuOnlyHeap->GetHeap();
	else if (m_type == ResourceType::cpuReadBack)
		pHeap = Gaia::Resources::cpuReadBackHeap->GetHeap();

	m_resource.CreateResource(
		device, pHeap, m_heapOffset, m_resourceDescription,
		initialState, clearValue
	);

	if (m_type != ResourceType::gpuOnly)
		m_resource.MapBuffer();
}

ID3D12Resource* D3DResourceView::GetResource() const noexcept {
	return m_resource.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS D3DResourceView::GetGPUAddress() const noexcept {
	return m_resource.Get()->GetGPUVirtualAddress();
}

std::uint8_t* D3DResourceView::GetCPUWPointer() const {
	if (m_type == ResourceType::gpuOnly)
		D3D_GENERIC_THROW("This buffer doesn't have CPU access.");

	return m_resource.GetCPUWPointer();
}
