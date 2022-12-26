#include <D3DHeap.hpp>
#include <algorithm>
#include <D3DHelperFunctions.hpp>

D3DHeap::D3DHeap(const Args& arguments)
	: m_heapType{ arguments.type.value() }, m_maxAlignment{ 0u }, m_totalHeapSize{ 0u } {}

void D3DHeap::CreateHeap(ID3D12Device* device) {
	D3D12_HEAP_PROPERTIES heapProp{};
	heapProp.Type = m_heapType;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1u;
	heapProp.VisibleNodeMask = 1u;

	D3D12_HEAP_DESC desc{};
	desc.SizeInBytes = Align(m_totalHeapSize, m_maxAlignment);
	desc.Alignment = m_maxAlignment;
	desc.Flags = D3D12_HEAP_FLAG_NONE;
	desc.Properties = heapProp;

	device->CreateHeap(&desc, IID_PPV_ARGS(&m_pHeap));
}

ID3D12Heap* D3DHeap::GetHeap() const noexcept {
	return m_pHeap.Get();
}

size_t D3DHeap::ReserveSizeAndGetOffset(size_t heapSize, UINT64 alignment) noexcept {
	m_totalHeapSize = Align(m_totalHeapSize, alignment);
	size_t offset = m_totalHeapSize;

	m_maxAlignment = std::max(m_maxAlignment, alignment);
	m_totalHeapSize += heapSize;

	return offset;
}
