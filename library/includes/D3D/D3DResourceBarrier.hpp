#ifndef D3D_RESOURCE_BARRIER_HPP_
#define D3D_RESOURCE_BARRIER_HPP_
#include <array>
#include <vector>
#include <cassert>
#include <D3DHeaders.hpp>

namespace Gaia
{
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
	const D3D12_RESOURCE_TRANSITION_BARRIER& GetTransition() const noexcept
	{
		return m_barrier.Transition;
	}

	[[nodiscard]]
	D3D12_RESOURCE_BARRIER Get() const noexcept { return m_barrier; }

private:
	D3D12_RESOURCE_BARRIER m_barrier;
};

template<UINT barrierCount = 1u>
// This one can be created and destroyed every frame.
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

	D3DResourceBarrier& AddBarrier(const ResourceBarrierBuilder& barrierBuilder)
	{
		return AddBarrier(barrierBuilder.Get());
	}

	void SetTransitionResource(size_t barrierIndex, ID3D12Resource* resource) noexcept
	{
		m_barriers[barrierIndex].Transition.pResource = resource;
	}

	void RecordBarriers(ID3D12GraphicsCommandList* commandList) const noexcept
	{
		commandList->ResourceBarrier(barrierCount, std::data(m_barriers));
	}

private:
	size_t                                           m_currentIndex;
	std::array<D3D12_RESOURCE_BARRIER, barrierCount> m_barriers;
};

// This one should not be created and destroyed on every frame.
class D3DResourceBarrier1_1
{
public:
	D3DResourceBarrier1_1() : m_barriers{} {}
	D3DResourceBarrier1_1(size_t totalBarrierCount) : D3DResourceBarrier1_1{}
	{
		m_barriers.reserve(totalBarrierCount);
	}

	D3DResourceBarrier1_1& AddBarrier(const D3D12_RESOURCE_BARRIER& barrier)
	{
		m_barriers.emplace_back(barrier);

		return *this;
	}

	D3DResourceBarrier1_1& AddBarrier(const ResourceBarrierBuilder& barrierBuilder)
	{
		return AddBarrier(barrierBuilder.Get());
	}

	void SetTransitionResource(size_t barrierIndex, ID3D12Resource* resource) noexcept
	{
		m_barriers[barrierIndex].Transition.pResource = resource;
	}

	void RecordBarriers(ID3D12GraphicsCommandList* commandList) const noexcept
	{
		commandList->ResourceBarrier(static_cast<UINT>(std::size(m_barriers)), std::data(m_barriers));
	}

	[[nodiscard]]
	std::uint32_t GetCount() const noexcept
	{
		return static_cast<std::uint32_t>(std::size(m_barriers));
	}

private:
	std::vector<D3D12_RESOURCE_BARRIER> m_barriers;

public:
	D3DResourceBarrier1_1(const D3DResourceBarrier1_1& other) noexcept
		: m_barriers{ other.m_barriers }
	{}
	D3DResourceBarrier1_1& operator=(const D3DResourceBarrier1_1& other) noexcept
	{
		m_barriers = other.m_barriers;

		return *this;
	}
	D3DResourceBarrier1_1(D3DResourceBarrier1_1&& other) noexcept
		: m_barriers{ std::move(other.m_barriers) }
	{}
	D3DResourceBarrier1_1& operator=(D3DResourceBarrier1_1&& other) noexcept
	{
		m_barriers = std::move(other.m_barriers);

		return *this;
	}
};
}
#endif
