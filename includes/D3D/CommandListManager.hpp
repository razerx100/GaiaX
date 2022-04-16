#ifndef COMMAND_LIST_MANAGER_HPP_
#define COMMAND_LIST_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <vector>

class CommandListManager {
public:
	CommandListManager(
		ID3D12Device4* device,
		D3D12_COMMAND_LIST_TYPE type, size_t allocatorsCount
	);

	void Reset(size_t allocIndex);
	void Close() const;

	[[nodiscard]]
	ID3D12GraphicsCommandList* GetCommandListRef() const noexcept;

private:
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pCommandAllocators;
};
#endif
