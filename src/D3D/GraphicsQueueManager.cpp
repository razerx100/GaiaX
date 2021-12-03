#include <GraphicsQueueManager.hpp>
#include <D3DThrowMacros.hpp>
#include <InstanceManager.hpp>

// Graphics Command Queue
GraphicsQueueManager::GraphicsQueueManager(
	ID3D12Device5* device,
	D3D12_COMMAND_LIST_TYPE type, std::uint8_t bufferCount
)
	: m_fenceEvent(nullptr),
	m_fenceValues(bufferCount, 0u) {

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = type;

	HRESULT hr;
	GFX_THROW_FAILED(hr, device->CreateCommandQueue(
		&queueDesc, __uuidof(ID3D12CommandQueue), &m_pCommandQueue
	));
}

ID3D12CommandQueue* GraphicsQueueManager::GetQueueRef() const noexcept {
	return m_pCommandQueue.Get();
}

void GraphicsQueueManager::InitSyncObjects(
	ID3D12Device5* device,
	std::uint32_t backBufferIndex
) {
	HRESULT hr;
	GFX_THROW_FAILED(hr, device->CreateFence(
		m_fenceValues[backBufferIndex],
		D3D12_FENCE_FLAG_NONE,
		__uuidof(ID3D12Fence),
		&m_pFence
	));
	m_fenceValues[backBufferIndex]++;

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
		GFX_THROW_FAILED(hr, HRESULT_FROM_WIN32(GetLastError()));
}

void GraphicsQueueManager::WaitForGPU(std::uint32_t backBufferIndex) {
	HRESULT hr;
	GFX_THROW_FAILED(hr, m_pCommandQueue->Signal(
		m_pFence.Get(), m_fenceValues[backBufferIndex])
	);

	GFX_THROW_FAILED(hr,
		m_pFence->SetEventOnCompletion(
			m_fenceValues[backBufferIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	m_fenceValues[backBufferIndex]++;
}

void GraphicsQueueManager::ExecuteCommandLists(
	ID3D12GraphicsCommandList* commandList
) const noexcept {
	ID3D12CommandList* const ppCommandLists[] = { commandList };

	m_pCommandQueue->ExecuteCommandLists(
		static_cast<std::uint32_t>(std::size(ppCommandLists)), ppCommandLists
	);
}

void GraphicsQueueManager::MoveToNextFrame(std::uint32_t backBufferIndex) {
	const std::uint64_t currentFenceValue = m_fenceValues[backBufferIndex];
	HRESULT hr;
	GFX_THROW_FAILED(hr,
		m_pCommandQueue->Signal(m_pFence.Get(),
			currentFenceValue)
	);

	backBufferIndex = SwapchainInst::GetRef()->GetCurrentBackBufferIndex();

	if (m_pFence->GetCompletedValue() < m_fenceValues[backBufferIndex]) {
		GFX_THROW_FAILED(hr,
			m_pFence->SetEventOnCompletion(
				m_fenceValues[backBufferIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	m_fenceValues[backBufferIndex] = currentFenceValue + 1;
}

void GraphicsQueueManager::ResetFenceValuesWith(std::uint32_t valueIndex) {
	for (std::uint32_t index = 0; index < m_fenceValues.size(); ++index)
		m_fenceValues[index] = m_fenceValues[valueIndex];
}
