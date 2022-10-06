#ifndef D3D_COMMAND_QUEUE_HPP_
#define D3D_COMMAND_QUEUE_HPP_
#include <D3DHeaders.hpp>
#include <vector>

class D3DCommandQueue {
public:
	D3DCommandQueue(
		ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type, size_t fenceCount = 1u
	);

	void InitSyncObjects(ID3D12Device* device, size_t initialFenceValueIndex = 0u);
	void WaitForGPU(size_t fenceValueIndex = 0u);
	void ExecuteCommandLists(ID3D12GraphicsCommandList* commandList) const noexcept;
	void MoveToNextFrame(size_t frameIndex);
	void ResetFenceValuesWith(size_t valueIndex) noexcept;

	[[nodiscard]]
	ID3D12CommandQueue* GetQueue() const noexcept;

private:
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	ComPtr<ID3D12Fence> m_pFence;
	std::vector<UINT64> m_fenceValues;
	HANDLE m_fenceEvent;
};
#endif
