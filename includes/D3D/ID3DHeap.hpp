#ifndef __I_D3D_HEAP_HPP__
#define __I_D3D_HEAP_HPP__
#include <D3DHeaders.hpp>

class ID3DHeap {
public:
	virtual ~ID3DHeap() = default;

	virtual void CreateHeap(
		ID3D12Device* device, size_t heapSize,
		bool msaa, bool uploadHeap = false
	) = 0;

	virtual ID3D12Heap* GetHeap() const noexcept = 0;
};
#endif
