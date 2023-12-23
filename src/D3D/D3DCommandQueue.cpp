#include <D3DCommandQueue.hpp>

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
