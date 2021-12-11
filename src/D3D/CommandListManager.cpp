#include <CommandListManager.hpp>
#include <D3DThrowMacros.hpp>

// CommandList
CommandListManager::CommandListManager(
	ID3D12Device5* device,
	D3D12_COMMAND_LIST_TYPE type, std::uint8_t allocatorsCount
) :
	m_pCommandAllocators(allocatorsCount) {

	HRESULT hr;
	for (std::uint32_t index = 0; index < allocatorsCount; ++index) {
		D3D_THROW_FAILED(hr,
			device->CreateCommandAllocator(
				type,
				__uuidof(ID3D12CommandAllocator),
				&m_pCommandAllocators[index]
			));
	}

	D3D_THROW_FAILED(hr,
		device->CreateCommandList1(
			0, type,
			D3D12_COMMAND_LIST_FLAG_NONE,
			__uuidof(ID3D12GraphicsCommandList),
			&m_pCommandList
		)
	);
}

void CommandListManager::Reset(std::uint32_t allocIndex) const {
	HRESULT hr;
	D3D_THROW_FAILED(hr, m_pCommandAllocators[allocIndex]->Reset());
	D3D_THROW_FAILED(hr,
		m_pCommandList->Reset(m_pCommandAllocators[allocIndex].Get(), nullptr)
	);
}

void CommandListManager::Close() const {
	HRESULT hr;
	D3D_THROW_FAILED(hr, m_pCommandList->Close());
}

ID3D12GraphicsCommandList* CommandListManager::GetCommandListRef() const noexcept {
	return m_pCommandList.Get();
}
