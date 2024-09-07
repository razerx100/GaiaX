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
void D3DCommandQueue::Create(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type, size_t bufferCount)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc
	{
		.Type  = type,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	};

	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));

	for (size_t index = 0u; index < bufferCount; ++index)
		m_commandLists.emplace_back(device, type);
}

void D3DCommandQueue::Signal(ID3D12Fence* fence, UINT64 signalValue) const
{
	m_commandQueue->Signal(fence, signalValue);
}

void D3DCommandQueue::Wait(ID3D12Fence* fence, UINT64 waitValue) const
{
	m_commandQueue->Wait(fence, waitValue);
}

void D3DCommandQueue::ExecuteCommandLists(
	ID3D12GraphicsCommandList* commandList
) const noexcept {
	ID3D12CommandList* const ppCommandList{ commandList };
	m_commandQueue->ExecuteCommandLists(1u, &ppCommandList);
}
