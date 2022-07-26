module;

#include <D3DThrowMacros.hpp>
#include <D3DHelperFunctions.hpp>

module D3DHeap;

D3DHeap::D3DHeap(D3D12_HEAP_TYPE type) noexcept : m_heapType(type) {}

void D3DHeap::CreateHeap(
	ID3D12Device* device, size_t heapSize, UINT64 alignment
) {
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = m_heapType;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1u;
	heapProp.VisibleNodeMask = 1u;

	D3D12_HEAP_DESC desc = {};
	desc.SizeInBytes = Align(heapSize, alignment);
	desc.Alignment = alignment;
	desc.Flags = D3D12_HEAP_FLAG_NONE;
	desc.Properties = heapProp;

	HRESULT hr;
	D3D_THROW_FAILED(hr,
		device->CreateHeap(&desc, __uuidof(ID3D12Heap), &m_pHeap)
	);
}

ID3D12Heap* D3DHeap::GetHeap() const noexcept {
	return m_pHeap.Get();
}
