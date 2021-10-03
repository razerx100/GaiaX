#ifndef __COMMAND_QUEUE_MANAGER_HPP__
#define __COMMAND_QUEUE_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <vector>
#include <cstdint>

class CommandListManager {
public:
	CommandListManager(D3D12_COMMAND_LIST_TYPE type, std::uint8_t allocatorsCount);

	void Reset();
	void Close();

	ID3D12GraphicsCommandList* GetCommandList() const noexcept;

private:
	std::uint8_t m_allocatorIndex;
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pCommandAllocators;
};

class CommandQueueManager {
public:
};
#endif
