#include <CommandQueueManager.hpp>
#include <D3DThrowMacros.hpp>
#include <SwapChainManager.hpp>

// CommandList
CommandListManager::CommandListManager(
	ID3D12Device5* device,
	D3D12_COMMAND_LIST_TYPE type, std::uint8_t allocatorsCount
) :
	m_pCommandAllocators(allocatorsCount) {

	HRESULT hr;
	for (std::uint32_t index = 0; index < allocatorsCount; ++index) {
		GFX_THROW_FAILED(hr,
			device->CreateCommandAllocator(
				type,
				__uuidof(ID3D12CommandAllocator),
				&m_pCommandAllocators[index]
			));
	}

	GFX_THROW_FAILED(hr,
		device->CreateCommandList1(
			0, type,
			D3D12_COMMAND_LIST_FLAG_NONE,
			__uuidof(ID3D12GraphicsCommandList),
			&m_pCommandList
		)
	);
}

void CommandListManager::Reset(std::uint32_t allocIndex) {
	HRESULT hr;
	GFX_THROW_FAILED(hr, m_pCommandAllocators[allocIndex]->Reset());
	GFX_THROW_FAILED(hr,
		m_pCommandList->Reset(m_pCommandAllocators[allocIndex].Get(), nullptr)
	);
}

void CommandListManager::Close() const {
	HRESULT hr;
	GFX_THROW_FAILED(hr, m_pCommandList->Close());
}

ID3D12GraphicsCommandList* CommandListManager::GetCommandList() const noexcept {
	return m_pCommandList.Get();
}

// CommandQueue
_CommandQueueManager::_CommandQueueManager(
	ID3D12Device5* device,
	D3D12_COMMAND_LIST_TYPE type, std::uint8_t bufferCount
)
	: m_commandListMan(device, type, bufferCount),
	m_fenceEvent(nullptr),
	m_fenceValues(bufferCount, 0u) {

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = type;

	HRESULT hr;
	GFX_THROW_FAILED(hr, device->CreateCommandQueue(
		&queueDesc, __uuidof(ID3D12CommandQueue), &m_pCommandQueue
	));
}

ID3D12CommandQueue* _CommandQueueManager::GetCommandQueueRef() const noexcept {
	return m_pCommandQueue.Get();
}

ID3D12GraphicsCommandList* _CommandQueueManager::GetCommandListRef() const noexcept {
	return m_commandListMan.GetCommandList();
}

void _CommandQueueManager::InitSyncObjects(
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

void _CommandQueueManager::WaitForGPU(std::uint32_t backBufferIndex) {
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

void _CommandQueueManager::ExecuteCommandLists() const noexcept {
	ID3D12CommandList* ppCommandLists[] = { m_commandListMan.GetCommandList() };

	m_pCommandQueue->ExecuteCommandLists(
		static_cast<std::uint32_t>(std::size(ppCommandLists)), ppCommandLists
	);
}

// Graphics Command Queue
void GraphicsCQueueManager::RecordCommandList() {
	std::uint32_t allocIndex = 0u;
	SwapChainManager* swapRef = GetSwapChainInstance();
	if (swapRef)
		allocIndex = swapRef->GetCurrentBackBufferIndex();

	m_commandListMan.Reset(allocIndex);
}

void GraphicsCQueueManager::CloseCommandList() const {
	m_commandListMan.Close();
}

void GraphicsCQueueManager::MoveToNextFrame(std::uint32_t backBufferIndex) {
	const std::uint64_t currentFenceValue = m_fenceValues[backBufferIndex];
	HRESULT hr;
	GFX_THROW_FAILED(hr,
		m_pCommandQueue->Signal(m_pFence.Get(),
			currentFenceValue)
	);

	backBufferIndex = GetSwapChainInstance()->GetCurrentBackBufferIndex();

	if (m_pFence->GetCompletedValue() < m_fenceValues[backBufferIndex]) {
		GFX_THROW_FAILED(hr,
			m_pFence->SetEventOnCompletion(
				m_fenceValues[backBufferIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	m_fenceValues[backBufferIndex] = currentFenceValue + 1;
}

void GraphicsCQueueManager::ResetFenceValuesWith(std::uint32_t valueIndex) {
	for (std::uint32_t index = 0; index < m_fenceValues.size(); ++index)
		m_fenceValues[index] = m_fenceValues[valueIndex];
}
