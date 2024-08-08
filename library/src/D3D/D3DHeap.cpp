#include <D3DHeap.hpp>
#include <D3DHelperFunctions.hpp>

D3DHeap::D3DHeap(ID3D12Device* device, D3D12_HEAP_TYPE type, UINT64 size, bool msaa /* = false */)
	: m_type{ type }, m_size{ size }, m_heap{}
{
	Allocate(device, type, size, msaa);
}

void D3DHeap::Allocate(ID3D12Device* device, D3D12_HEAP_TYPE type, UINT64 size, bool msaa)
{
	D3D12_HEAP_PROPERTIES heapProperties{
		.Type                 = type,
		.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask     = 1u,
		.VisibleNodeMask      = 1u
	};

	UINT64 alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

	if (msaa)
		alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;

	// Might want to set the aligned size as m_size. But need to test
	// if I can use it all first.
	const UINT64 alignedSize = Align(size, alignment);

	D3D12_HEAP_DESC desc{
		.SizeInBytes = alignedSize,
		.Properties  = heapProperties,
		.Alignment   = alignment,
		.Flags       = D3D12_HEAP_FLAG_NONE
	};

	device->CreateHeap(&desc, IID_PPV_ARGS(&m_heap));
}
