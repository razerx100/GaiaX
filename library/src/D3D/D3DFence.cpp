#include <D3DFence.hpp>

void D3DFence::Create(UINT64 initialValue/* = 0u */)
{
	m_device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));

	m_fenceCPUEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void D3DFence::Signal(UINT64 fenceValue) const
{
	m_fence->Signal(fenceValue);
}

void D3DFence::Wait(UINT64 waitValue) const
{
	m_fence->SetEventOnCompletion(waitValue, m_fenceCPUEvent);
	WaitForSingleObjectEx(m_fenceCPUEvent, INFINITE, FALSE);
}

UINT64 D3DFence::GetCurrentValue() const noexcept
{
	return m_fence->GetCompletedValue();
}
