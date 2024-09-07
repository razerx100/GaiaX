#include <D3DCommandQueue.hpp>
#include <Exception.hpp>

// D3D CommandList
D3DCommandList::D3DCommandList(
		ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type, bool cmdList6, size_t allocatorCount
) : m_pCommandAllocators{ allocatorCount }
{
	for (auto& commandAllocator : m_pCommandAllocators)
		device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));

	device->CreateCommandList1(
		0u, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_pCommandList)
	);

	if (cmdList6)
	{
		HRESULT hr = m_pCommandList.As(&m_pCommandList6);

		if (hr == E_NOINTERFACE)
			throw Exception("Interface Query Error", "CommandList6 is not supported.");
	}
}

void D3DCommandList::Reset(size_t allocatorIndex) {
	ComPtr<ID3D12CommandAllocator>& currentAllocator = m_pCommandAllocators[allocatorIndex];

	currentAllocator->Reset();
	m_pCommandList->Reset(currentAllocator.Get(), nullptr);
}

void D3DCommandList::ResetFirst() {
	Reset(0u);
}

void D3DCommandList::Close() const {
	m_pCommandList->Close();
}

ID3D12GraphicsCommandList* D3DCommandList::GetCommandList() const noexcept {
	return m_pCommandList.Get();
}

ID3D12GraphicsCommandList6* D3DCommandList::GetCommandList6() const noexcept {
	return m_pCommandList6.Get();
}


// D3D Command Queue
D3DCommandQueue::D3DCommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc
	{
		.Type  = type,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	};

	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue));
}

void D3DCommandQueue::SignalCommandQueue(ID3D12Fence* fence, UINT64 fenceValue) const {
	m_pCommandQueue->Signal(fence, fenceValue);
}

void D3DCommandQueue::WaitOnGPU(ID3D12Fence* fence, UINT64 fenceValue) const {
	m_pCommandQueue->Wait(fence, fenceValue);
}

void D3DCommandQueue::ExecuteCommandLists(
	ID3D12GraphicsCommandList* commandList
) const noexcept {
	ID3D12CommandList* const ppCommandList{ commandList };
	m_pCommandQueue->ExecuteCommandLists(1u, &ppCommandList);
}

ID3D12CommandQueue* D3DCommandQueue::GetQueue() const noexcept {
	return m_pCommandQueue.Get();
}
