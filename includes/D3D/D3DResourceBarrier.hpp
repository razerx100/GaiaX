#ifndef D3D_RESOURCE_BARRIER_HPP_
#define D3D_RESOURCE_BARRIER_HPP_
#include <array>
#include <cassert>
#include <D3DHeaders.hpp>
#include <D3DHelperFunctions.hpp>

template<UINT barrierCount = 1u>
class D3DResourceBarrier {
public:
	D3DResourceBarrier() noexcept : m_currentIndex{ 0u } {}

	[[nodiscard]]
	D3DResourceBarrier& AddBarrier(
		ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState
	) noexcept {
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");
		m_barriers[m_currentIndex] = GetTransitionBarrier(resource, beforeState, afterState);
		++m_currentIndex;

		return *this;
	}

	void RecordBarriers(ID3D12GraphicsCommandList* commandList) noexcept {
		commandList->ResourceBarrier(barrierCount, std::data(m_barriers));
	}

private:
	UINT m_currentIndex;
	std::array<D3D12_RESOURCE_BARRIER, barrierCount> m_barriers;
};
#endif
