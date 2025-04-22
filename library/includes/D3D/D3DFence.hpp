#ifndef D3D_FENCE_HPP_
#define D3D_FENCE_HPP_
#include <D3DHeaders.hpp>
#include <utility>

namespace Gaia
{
class D3DFence
{
public:
	D3DFence() : m_fence{}, m_fenceCPUEvent{ nullptr } {}

	void Create(ID3D12Device* device, UINT64 initialValue = 0u);

	void Signal(UINT64 signalValue) const;
	void Wait(UINT64 waitValue) const;

	[[nodiscard]]
	UINT64 GetCurrentValue() const noexcept;
	[[nodiscard]]
	ID3D12Fence* Get() const noexcept { return m_fence.Get(); }

private:
	ComPtr<ID3D12Fence> m_fence;
	HANDLE              m_fenceCPUEvent;

public:
	D3DFence(const D3DFence&) = delete;
	D3DFence& operator=(const D3DFence&) = delete;

	D3DFence(D3DFence&& other) noexcept
		: m_fence{ std::move(other.m_fence) },
		m_fenceCPUEvent{ std::exchange(other.m_fenceCPUEvent, nullptr) }
	{}
	D3DFence& operator=(D3DFence&& other) noexcept
	{
		m_fence         = std::move(other.m_fence);
		m_fenceCPUEvent = std::exchange(other.m_fenceCPUEvent, nullptr);

		return *this;
	}
};
}
#endif
