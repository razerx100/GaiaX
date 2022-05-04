#ifndef GRAPHICS_QUEUE_MANAGER_HPP_
#define GRAPHICS_QUEUE_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <vector>

class GraphicsQueueManager {
public:
	GraphicsQueueManager(
		ID3D12Device* device, size_t bufferCount
	);

	void InitSyncObjects(ID3D12Device* device, size_t backBufferIndex);
	void WaitForGPU(size_t backBufferIndex);
	void ExecuteCommandLists(
		ID3D12GraphicsCommandList* commandList
	) const noexcept;
	void MoveToNextFrame(size_t backBufferIndex);
	void ResetFenceValuesWith(size_t valueIndex);

	[[nodiscard]]
	ID3D12CommandQueue* GetQueueRef() const noexcept;

private:
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;

	ComPtr<ID3D12Fence> m_pFence;
	HANDLE m_fenceEvent;
	std::vector<std::uint64_t> m_fenceValues;
	const std::size_t m_bufferCount;
};
#endif
