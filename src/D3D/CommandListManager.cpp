#include <CommandListManager.hpp>
#include <D3DThrowMacros.hpp>

// CommandList
CommandListManager::CommandListManager(
	ID3D12Device4* device,
	D3D12_COMMAND_LIST_TYPE type, size_t allocatorsCount
) :
	m_pCommandAllocators(allocatorsCount) {

	HRESULT hr;
	for (size_t index = 0u; index < allocatorsCount; ++index) {
		D3D_THROW_FAILED(hr,
			device->CreateCommandAllocator(
				type,
				__uuidof(ID3D12CommandAllocator),
				&m_pCommandAllocators[index]
			));
	}

	D3D_THROW_FAILED(hr,
		device->CreateCommandList1(
			0u, type,
			D3D12_COMMAND_LIST_FLAG_NONE,
			__uuidof(ID3D12GraphicsCommandList),
			&m_pCommandList
		)
	);
}

void CommandListManager::Reset(size_t allocIndex) {
	ComPtr<ID3D12CommandAllocator>& currentAllocator = m_pCommandAllocators[allocIndex];

	HRESULT hr;
	D3D_THROW_FAILED(hr, currentAllocator->Reset());
	D3D_THROW_FAILED(hr,
		m_pCommandList->Reset(currentAllocator.Get(), nullptr)
	);
}

void CommandListManager::Close() const {
	HRESULT hr;
	D3D_THROW_FAILED(hr, m_pCommandList->Close());
}

ID3D12GraphicsCommandList* CommandListManager::GetCommandListRef() const noexcept {
	return m_pCommandList.Get();
}
