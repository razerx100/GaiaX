#ifndef D3D_COMMAND_LIST_HPP_
#define D3D_COMMAND_LIST_HPP_
#include <D3DHeaders.hpp>
#include <vector>

class D3DCommandList {
public:
	D3DCommandList(
		ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type, size_t allocatorCount = 1u
	);

	void Reset(size_t allocatorIndex = 0u);
	void Close() const;

	[[nodiscard]]
	ID3D12GraphicsCommandList* GetCommandList() const noexcept;

private:
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pCommandAllocators;
};
#endif
