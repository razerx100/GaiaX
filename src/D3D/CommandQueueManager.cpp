#include <CommandQueueManager.hpp>

CommandListManager::CommandListManager(
	D3D12_COMMAND_LIST_TYPE type, std::uint8_t allocatorsCount
) : m_allocatorIndex(0u), m_pCommandAllocators(allocatorsCount) {

}

void CommandListManager::Reset() {

}

void CommandListManager::Close() {

}

ID3D12GraphicsCommandList* CommandListManager::GetCommandList() const noexcept {
	return m_pCommandList.Get();
}
