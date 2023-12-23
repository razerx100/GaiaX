#ifndef D3D_HEAP_HPP_
#define D3D_HEAP_HPP_
#include <D3DHeaders.hpp>
#include <optional>

class D3DHeap
{
public:
	D3DHeap(D3D12_HEAP_TYPE type);

	void CreateHeap(ID3D12Device* device);

	[[nodiscard]]
	size_t ReserveSizeAndGetOffset(size_t heapSize, UINT64 alignment) noexcept;

	[[nodiscard]]
	ID3D12Heap* GetHeap() const noexcept;

private:
	D3D12_HEAP_TYPE m_heapType;
	UINT64 m_maxAlignment;
	UINT64 m_totalHeapSize;
	ComPtr<ID3D12Heap> m_pHeap;
};
#endif
