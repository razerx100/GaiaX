#include <D3DFence.hpp>
#include <D3DThrowMacros.hpp>

D3DFence::D3DFence(ID3D12Device* device, size_t fenceValueCount)
	: m_fenceValues(std::deque<UINT64>{fenceValueCount, 0u}), m_fenceCPUEvent{nullptr} {
	HRESULT hr{};
	D3D_THROW_FAILED(hr,
		device->CreateFence(GetFrontValue(), D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))
	);
	IncreaseFrontValue(0u);

	m_fenceCPUEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceCPUEvent == nullptr)
		D3D_THROW_FAILED(hr, HRESULT_FROM_WIN32(GetLastError()));
}

void D3DFence::IncreaseFrontValue(UINT64 oldValue) noexcept {
	m_fenceValues.front() = oldValue + 1u;
}

void D3DFence::AdvanceValueInQueue() noexcept {
	UINT64 currentValue = GetFrontValue();
	m_fenceValues.pop();
	m_fenceValues.push(currentValue);
}

void D3DFence::WaitOnCPUConditional() {
	UINT64 currentValue = GetFrontValue();
	if (m_fence->GetCompletedValue() < currentValue)
		WaitOnCPU(currentValue);
}

void D3DFence::SignalFence(UINT64 fenceValue) const {
	HRESULT hr{};
	D3D_THROW_FAILED(hr, m_fence->Signal(fenceValue));
}

void D3DFence::WaitOnCPU() {
	WaitOnCPU(GetFrontValue());
}

void D3DFence::WaitOnCPU(UINT64 fenceValue) {
	HRESULT hr{};

	D3D_THROW_FAILED(hr, m_fence->SetEventOnCompletion(fenceValue, m_fenceCPUEvent));
	WaitForSingleObjectEx(m_fenceCPUEvent, INFINITE, FALSE);
}

void D3DFence::ResetFenceValues(UINT64 value) noexcept {
	const size_t fenceCount = std::size(m_fenceValues);
	for (size_t _ = 0u; _ < fenceCount; ++_) {
		m_fenceValues.pop();
		m_fenceValues.push(value);
	}
}

UINT64 D3DFence::GetFrontValue() const noexcept {
	return m_fenceValues.front();
}

ID3D12Fence* D3DFence::GetFence() const noexcept {
	return m_fence.Get();
}
