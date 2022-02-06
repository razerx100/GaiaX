#ifndef __D3D_HEAP_HPP__
#define __D3D_HEAP_HPP__
#include <ID3DHeap.hpp>

class D3DHeap : public ID3DHeap {
public:
	void CreateHeap(
		ID3D12Device* device, size_t heapSize,
		bool msaa, bool uploadHeap = false
	) override;

	ID3D12Heap* GetHeap() const noexcept override;

private:
	ComPtr<ID3D12Heap> m_pHeap;
};
#endif
