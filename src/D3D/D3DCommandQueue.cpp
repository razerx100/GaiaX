#include <D3DCommandQueue.hpp>
#include <D3DThrowMacros.hpp>

D3DCommandQueue::D3DCommandQueue(
	ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, size_t fenceCount
) : m_fenceValues{ fenceCount, 0u }, m_fenceEvent{ nullptr } {
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = type;

	HRESULT hr{};
	D3D_THROW_FAILED(hr,
		device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue))
	);
}

void D3DCommandQueue::InitSyncObjects(ID3D12Device* device, size_t initialFenceValueIndex) {
	HRESULT hr{};
	D3D_THROW_FAILED(hr,
		device->CreateFence(
			m_fenceValues[initialFenceValueIndex], D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&m_pFence)
		)
	);
	++m_fenceValues[initialFenceValueIndex];

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
		D3D_THROW_FAILED(hr, HRESULT_FROM_WIN32(GetLastError()));
}

void D3DCommandQueue::WaitForGPU(size_t fenceValueIndex) {
	UINT64& currentFenceValue = m_fenceValues[fenceValueIndex];

	HRESULT hr{};
	D3D_THROW_FAILED(hr, m_pCommandQueue->Signal(m_pFence.Get(), currentFenceValue));

	D3D_THROW_FAILED(hr, m_pFence->SetEventOnCompletion(currentFenceValue, m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	++currentFenceValue;
}

void D3DCommandQueue::ExecuteCommandLists(
	ID3D12GraphicsCommandList* commandList
) const noexcept {
	ID3D12CommandList* const ppCommandList{ commandList };
	m_pCommandQueue->ExecuteCommandLists(1u, &ppCommandList);
}

void D3DCommandQueue::MoveToNextFrame(size_t frameIndex) {
	const UINT64 oldFenceValue = m_fenceValues[frameIndex];

	HRESULT hr{};
	D3D_THROW_FAILED(hr, m_pCommandQueue->Signal(m_pFence.Get(), oldFenceValue));

	const static size_t bufferCount = std::size(m_fenceValues);

	++frameIndex;
	std::size_t newBackBufferIndex =
		frameIndex < bufferCount ? frameIndex : frameIndex % bufferCount;

	UINT64& newFenceValue = m_fenceValues[newBackBufferIndex];

	if (m_pFence->GetCompletedValue() < newFenceValue) {
		D3D_THROW_FAILED(hr, m_pFence->SetEventOnCompletion(newFenceValue, m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	newFenceValue = oldFenceValue + 1u;
}

void D3DCommandQueue::ResetFenceValuesWith(size_t valueIndex) noexcept {
	for (auto& fenceValue : m_fenceValues)
		fenceValue = m_fenceValues[valueIndex];
}

ID3D12CommandQueue* D3DCommandQueue::GetQueue() const noexcept {
	return m_pCommandQueue.Get();
}
