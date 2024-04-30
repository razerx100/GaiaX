#ifndef D3D_FENCE_HPP_
#define D3D_FENCE_HPP_
#include <D3DHeaders.hpp>
#include <queue>
#include <optional>

class D3DFence {
public:
	D3DFence(ID3D12Device* device, size_t fenceValueCount = 1u);

	void AdvanceValueInQueue() noexcept;
	void WaitOnCPU();
	void WaitOnCPUConditional();
	void IncreaseFrontValue(UINT64 oldValue) noexcept;

	void SignalFence(UINT64 fenceValue) const;
	void ResetFenceValues(UINT64 value) noexcept;

	[[nodiscard]]
	ID3D12Fence* GetFence() const noexcept;
	[[nodiscard]]
	UINT64 GetFrontValue() const noexcept;

private:
	void WaitOnCPU(UINT64 fenceValue);

private:
	ComPtr<ID3D12Fence> m_fence;
	std::queue<UINT64> m_fenceValues;
	HANDLE m_fenceCPUEvent;
};
#endif
