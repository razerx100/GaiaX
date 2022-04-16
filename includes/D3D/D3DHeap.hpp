#ifndef D3D_HEAP_HPP_
#define D3D_HEAP_HPP_
#include <D3DHeaders.hpp>

class D3DHeap {
public:
	void CreateHeap(
		ID3D12Device* device, size_t heapSize,
		bool msaa, bool uploadHeap = false
	);

	[[nodiscard]]
	ID3D12Heap* GetHeap() const noexcept;

private:
	ComPtr<ID3D12Heap> m_pHeap;
};
#endif
