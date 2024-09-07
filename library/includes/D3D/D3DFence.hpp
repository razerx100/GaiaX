#ifndef D3D_FENCE_HPP_
#define D3D_FENCE_HPP_
#include <D3DHeaders.hpp>
#include <utility>

class D3DFence
{
public:
	D3DFence(ID3D12Device* device) : m_device{ device }, m_fence{}, m_fenceCPUEvent{ nullptr } {}

	void Create(UINT64 initialValue = 0u);

	void Signal(UINT64 signalValue) const;
	void Wait(UINT64 waitValue) const;

	[[nodiscard]]
	UINT64 GetCurrentValue() const noexcept;
	[[nodiscard]]
	ID3D12Fence* Get() const noexcept { return m_fence.Get(); }

private:
	ID3D12Device*       m_device;
	ComPtr<ID3D12Fence> m_fence;
	HANDLE              m_fenceCPUEvent;

public:
	D3DFence(const D3DFence&) = delete;
	D3DFence& operator=(const D3DFence&) = delete;

	D3DFence(D3DFence&& other) noexcept
		: m_device{ other.m_device }, m_fence{ std::move(other.m_fence) },
		m_fenceCPUEvent{ std::exchange(other.m_fenceCPUEvent, nullptr) }
	{}
	D3DFence& operator=(D3DFence&& other) noexcept
	{
		m_device        = other.m_device;
		m_fence         = std::move(other.m_fence);
		m_fenceCPUEvent = std::exchange(other.m_fenceCPUEvent, nullptr);

		return *this;
	}
};
#endif
