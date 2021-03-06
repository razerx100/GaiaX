#include <GraphicsQueueManager.hpp>
#include <D3DThrowMacros.hpp>

// Graphics Command Queue
GraphicsQueueManager::GraphicsQueueManager(
	ID3D12Device* device, size_t bufferCount
)
	: m_fenceEvent(nullptr),
	m_fenceValues(bufferCount, 0u), m_bufferCount(bufferCount) {

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT hr;
	D3D_THROW_FAILED(hr, device->CreateCommandQueue(
		&queueDesc, __uuidof(ID3D12CommandQueue), &m_pCommandQueue
	));
}

ID3D12CommandQueue* GraphicsQueueManager::GetQueueRef() const noexcept {
	return m_pCommandQueue.Get();
}

void GraphicsQueueManager::InitSyncObjects(
	ID3D12Device* device,
	size_t backBufferIndex
) {
	HRESULT hr;
	D3D_THROW_FAILED(hr, device->CreateFence(
		m_fenceValues[backBufferIndex],
		D3D12_FENCE_FLAG_NONE,
		__uuidof(ID3D12Fence),
		&m_pFence
	));
	++m_fenceValues[backBufferIndex];

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
		D3D_THROW_FAILED(hr, HRESULT_FROM_WIN32(GetLastError()));
}

void GraphicsQueueManager::WaitForGPU(size_t backBufferIndex) {
	std::uint64_t& currentFenceValue = m_fenceValues[backBufferIndex];

	HRESULT hr;
	D3D_THROW_FAILED(hr,
		m_pCommandQueue->Signal(
			m_pFence.Get(), currentFenceValue
		)
	);

	D3D_THROW_FAILED(hr,
		m_pFence->SetEventOnCompletion(
			currentFenceValue, m_fenceEvent
		)
	);
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	++currentFenceValue;
}

void GraphicsQueueManager::ExecuteCommandLists(
	ID3D12GraphicsCommandList* commandList
) const noexcept {
	ID3D12CommandList* const ppCommandLists[] = { commandList };

	m_pCommandQueue->ExecuteCommandLists(
		static_cast<UINT>(std::size(ppCommandLists)), ppCommandLists
	);
}

void GraphicsQueueManager::MoveToNextFrame(size_t backBufferIndex) {
	const std::uint64_t oldFenceValue = m_fenceValues[backBufferIndex];

	HRESULT hr;
	D3D_THROW_FAILED(hr,
		m_pCommandQueue->Signal(m_pFence.Get(),
			oldFenceValue
		)
	);

	++backBufferIndex;
	std::size_t newBackBufferIndex =
		backBufferIndex >= m_bufferCount ? backBufferIndex % m_bufferCount : backBufferIndex;

	std::uint64_t& newFenceValue = m_fenceValues[newBackBufferIndex];

	if (m_pFence->GetCompletedValue() < newFenceValue) {
		D3D_THROW_FAILED(hr,
			m_pFence->SetEventOnCompletion(
				newFenceValue, m_fenceEvent
			)
		);
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	newFenceValue = oldFenceValue + 1u;
}

void GraphicsQueueManager::ResetFenceValuesWith(size_t valueIndex) {
	for (size_t index = 0; index < std::size(m_fenceValues); ++index)
		m_fenceValues[index] = m_fenceValues[valueIndex];
}
