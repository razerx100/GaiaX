#ifndef D3D_COMMAND_LIST_HPP_
#define D3D_COMMAND_LIST_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <optional>

class D3DCommandList {
public:
	struct Args {
		std::optional<ID3D12Device4*> device;
		std::optional<D3D12_COMMAND_LIST_TYPE> type;
		std::optional<bool> cmdList6;
		std::optional<size_t> allocatorCount = 1u;
	};

public:
	D3DCommandList(const Args& arguments);

	void Reset(size_t allocatorIndex);
	void ResetFirst();
	void Close() const;

	[[nodiscard]]
	ID3D12GraphicsCommandList* GetCommandList() const noexcept;
	[[nodiscard]]
	ID3D12GraphicsCommandList6* GetCommandList6() const noexcept;

private:
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	ComPtr<ID3D12GraphicsCommandList6> m_pCommandList6;
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pCommandAllocators;
};
#endif
