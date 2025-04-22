#ifndef D3D_HEAP_HPP_
#define D3D_HEAP_HPP_
#include <D3DHeaders.hpp>
#include <utility>

namespace Gaia
{
class D3DHeap
{
public:
	D3DHeap(ID3D12Device* device, D3D12_HEAP_TYPE type, UINT64 size, bool msaa = false);

	[[nodiscard]]
	ID3D12Heap* Get() const noexcept { return m_heap.Get(); }
	[[nodiscard]]
	D3D12_HEAP_TYPE Type() const noexcept { return m_type; }
	[[nodiscard]]
	UINT64 Size() const noexcept { return m_size; }

private:
	void Allocate(ID3D12Device* device, D3D12_HEAP_TYPE type, UINT64 size, bool msaa);

private:
	D3D12_HEAP_TYPE    m_type;
	UINT64             m_size;
	ComPtr<ID3D12Heap> m_heap;

public:
	D3DHeap(const D3DHeap&) = delete;
	D3DHeap& operator=(const D3DHeap&) = delete;

	D3DHeap(D3DHeap&& other) noexcept
		: m_type{ other.m_type }, m_size{ other.m_size }, m_heap{ std::move(other.m_heap) }
	{}
	D3DHeap& operator=(D3DHeap&& other) noexcept
	{
		m_type = other.m_type;
		m_size = other.m_size;
		m_heap = std::move(other.m_heap);

		return *this;
	}
};
}
#endif
