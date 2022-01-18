#include <CopyQueueManager.hpp>
#include <D3DThrowMacros.hpp>

CopyQueueManager::CopyQueueManager(ID3D12Device* device)
	: m_fenceEvent(nullptr), m_fenceValue(0u) {

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT hr;
	D3D_THROW_FAILED(hr, device->CreateCommandQueue(
		&queueDesc, __uuidof(ID3D12CommandQueue), &m_pCommandQueue
	));
}

void CopyQueueManager::InitSyncObjects(ID3D12Device* device) {
	HRESULT hr;
	D3D_THROW_FAILED(hr, device->CreateFence(
		m_fenceValue,
		D3D12_FENCE_FLAG_NONE,
		__uuidof(ID3D12Fence),
		&m_pFence
	));
	++m_fenceValue;

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
		D3D_THROW_FAILED(hr, HRESULT_FROM_WIN32(GetLastError()));
}

void CopyQueueManager::WaitForGPU() {
	HRESULT hr;
	D3D_THROW_FAILED(hr, m_pCommandQueue->Signal(
		m_pFence.Get(), m_fenceValue)
	);

	D3D_THROW_FAILED(hr,
		m_pFence->SetEventOnCompletion(
			m_fenceValue, m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	++m_fenceValue;
}

void CopyQueueManager::ExecuteCommandLists(
	ID3D12GraphicsCommandList* commandList
) const noexcept {
	ID3D12CommandList* const ppCommandLists[] = { commandList };

	m_pCommandQueue->ExecuteCommandLists(
		static_cast<UINT>(std::size(ppCommandLists)), ppCommandLists
	);
}

ID3D12CommandQueue* CopyQueueManager::GetQueueRef() const noexcept {
	return m_pCommandQueue.Get();
}
