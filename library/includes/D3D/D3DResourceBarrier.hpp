#ifndef D3D_RESOURCE_BARRIER_HPP_
#define D3D_RESOURCE_BARRIER_HPP_
#include <array>
#include <cassert>
#include <D3DHeaders.hpp>

class ResourceBarrierBuilder
{
public:
	ResourceBarrierBuilder() : m_barrier{ .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE } {}

	ResourceBarrierBuilder& Transition(
		ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState, UINT subresourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
	) noexcept {
		m_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		D3D12_RESOURCE_TRANSITION_BARRIER& transition = m_barrier.Transition;
		transition.pResource   = resource;
		transition.Subresource = subresourceIndex;
		transition.StateBefore = beforeState;
		transition.StateAfter  = afterState;

		return *this;
	}

	ResourceBarrierBuilder& Aliasing(
		ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter
	) noexcept {
		m_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;

		D3D12_RESOURCE_ALIASING_BARRIER& aliasing = m_barrier.Aliasing;
		aliasing.pResourceBefore = resourceBefore;
		aliasing.pResourceAfter  = resourceAfter;

		return *this;
	}

	ResourceBarrierBuilder& UAV(ID3D12Resource* resource) noexcept
	{
		m_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;

		D3D12_RESOURCE_UAV_BARRIER& uav = m_barrier.UAV;
		uav.pResource = resource;

		return *this;
	}

	[[nodiscard]]
	D3D12_RESOURCE_BARRIER Get() const noexcept { return m_barrier; }

private:
	D3D12_RESOURCE_BARRIER m_barrier;
};

template<UINT barrierCount = 1u>
class D3DResourceBarrier
{
public:
	D3DResourceBarrier() : m_currentIndex{ 0u }, m_barriers{} {}

	[[nodiscard]]
	D3DResourceBarrier& AddBarrier(const D3D12_RESOURCE_BARRIER& barrier)
	{
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}

	[[nodiscard]]
	D3DResourceBarrier& AddBarrier(const ResourceBarrierBuilder& barrierBuilder)
	{
		return AddBarrier(barrierBuilder.Get());
	}

	void RecordBarriers(ID3D12GraphicsCommandList* commandList) const noexcept
	{
		commandList->ResourceBarrier(barrierCount, std::data(m_barriers));
	}

private:
	size_t                                           m_currentIndex;
	std::array<D3D12_RESOURCE_BARRIER, barrierCount> m_barriers;
};
#endif
