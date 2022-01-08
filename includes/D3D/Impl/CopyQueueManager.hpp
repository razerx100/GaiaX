#ifndef __COPY_QUEUE_MANAGER_HPP__
#define __COPY_QUEUE_MANAGER_HPP__
#include <ICopyQueueManager.hpp>

class CopyQueueManager : public ICopyQueueManager {
public:
	CopyQueueManager(ID3D12Device* device);

	void InitSyncObjects(ID3D12Device* device) override;
	void WaitForGPU() override;
	void ExecuteCommandLists(
		ID3D12GraphicsCommandList* commandList
	) const noexcept override;

	ID3D12CommandQueue* GetQueueRef() const noexcept override;

private:
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;

	ComPtr<ID3D12Fence> m_pFence;
	HANDLE m_fenceEvent;
	std::uint64_t m_fenceValue;
};
#endif