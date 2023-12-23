#ifndef D3D_COMMAND_QUEUE_HPP_
#define D3D_COMMAND_QUEUE_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <optional>

class D3DCommandQueue
{
public:
	D3DCommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

	void SignalCommandQueue(ID3D12Fence* fence, UINT64 fenceValue) const;
	void WaitOnGPU(ID3D12Fence* fence, UINT64 fenceValue) const;
	void ExecuteCommandLists(ID3D12GraphicsCommandList* commandList) const noexcept;

	[[nodiscard]]
	ID3D12CommandQueue* GetQueue() const noexcept;

private:
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
};
#endif
