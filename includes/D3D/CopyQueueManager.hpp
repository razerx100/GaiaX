#ifndef COPY_QUEUE_MANAGER_HPP_
#define COPY_QUEUE_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <cstdint>

class CopyQueueManager {
public:
	CopyQueueManager(ID3D12Device* device);

	void InitSyncObjects(ID3D12Device* device);
	void WaitForGPU();
	void ExecuteCommandLists(
		ID3D12GraphicsCommandList* commandList
	) const noexcept;

	[[nodiscard]]
	ID3D12CommandQueue* GetQueueRef() const noexcept;

private:
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;

	ComPtr<ID3D12Fence> m_pFence;
	HANDLE m_fenceEvent;
	std::uint64_t m_fenceValue;
};
#endif