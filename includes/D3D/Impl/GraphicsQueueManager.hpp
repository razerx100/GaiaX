#ifndef __GRAPHICS_QUEUE_MANAGER_HPP__
#define __GRAPHICS_QUEUE_MANAGER_HPP__
#include <IGraphicsQueueManager.hpp>
#include <vector>

class GraphicsQueueManager : public IGraphicsQueueManager {
public:
	GraphicsQueueManager(
		ID3D12Device* device, size_t bufferCount
	);

	void InitSyncObjects(ID3D12Device* device, size_t backBufferIndex) override;
	void WaitForGPU(size_t backBufferIndex) override;
	void ExecuteCommandLists(
		ID3D12GraphicsCommandList* commandList
	) const noexcept override;
	void MoveToNextFrame(size_t backBufferIndex) override;
	void ResetFenceValuesWith(size_t valueIndex) override;

	ID3D12CommandQueue* GetQueueRef() const noexcept override;

private:
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;

	ComPtr<ID3D12Fence> m_pFence;
	HANDLE m_fenceEvent;
	std::vector<std::uint64_t> m_fenceValues;
};
#endif
