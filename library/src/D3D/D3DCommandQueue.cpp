#include <D3DCommandQueue.hpp>

// D3D CommandList
D3DCommandList::D3DCommandList(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type) : D3DCommandList{}
{
	Create(device, type);
}

void D3DCommandList::Create(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type)
{
	device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_commandAllocator));

	ComPtr<ID3D12GraphicsCommandList> commandList{};

	device->CreateCommandList1(
		0u, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_commandList)
	);
}

void D3DCommandList::Reset() const
{
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);
}

void D3DCommandList::Close() const
{
	m_commandList->Close();
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
