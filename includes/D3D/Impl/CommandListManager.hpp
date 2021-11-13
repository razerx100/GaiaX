#ifndef __COMMAND_LIST_MANAGER_HPP__
#define __COMMAND_LIST_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <vector>

class CommandListManager {
public:
	CommandListManager(
		ID3D12Device5* device,
		D3D12_COMMAND_LIST_TYPE type, std::uint8_t allocatorsCount
	);

	void Reset(std::uint32_t allocIndex);
	void Close() const;

	ID3D12GraphicsCommandList* GetCommandList() const noexcept;

private:
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pCommandAllocators;
};
#endif
