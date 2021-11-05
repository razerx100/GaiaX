#ifndef __COMMAND_QUEUE_MANAGER_HPP__
#define __COMMAND_QUEUE_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <vector>
#include <cstdint>

class CommandListManager {
public:
	CommandListManager(
		ID3D12Device5* device,
		D3D12_COMMAND_LIST_TYPE type, std::uint8_t allocatorsCount
	);

	void Reset();
	void Close() const;

	ID3D12GraphicsCommandList* GetCommandList() const noexcept;

private:
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pCommandAllocators;
};

class CommandQueueManager {
public:
	CommandQueueManager(
		ID3D12Device5* device,
		D3D12_COMMAND_LIST_TYPE type, std::uint8_t bufferCount
	);

	void InitSyncObjects(ID3D12Device5* device, std::uint32_t backBufferIndex);

	void RecordCommandList();
	void CloseCommandList() const;
	void WaitForGPU(std::uint32_t backBufferIndex);
	void MoveToNextFrame(std::uint32_t backBufferIndex);

	ID3D12CommandQueue* GetCommandQueueRef() const noexcept;
	ID3D12GraphicsCommandList* GetCommandListRef() const noexcept;
	void ExecuteCommandLists() const noexcept;

private:
	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	CommandListManager m_commandListMan;

	ComPtr<ID3D12Fence> m_pFence;
	HANDLE m_fenceEvent;
	std::vector<std::uint64_t> m_fenceValues;
};

CommandQueueManager* GetGraphicsQueueInstance() noexcept;
void InitGraphicsQueueInstance(
	ID3D12Device5* device,
	std::uint8_t bufferCount
);
void CleanUpGraphicsQueueInstance() noexcept;
#endif
