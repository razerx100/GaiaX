#include <D3DCommandList.hpp>
#include <D3DThrowMacros.hpp>

D3DCommandList::D3DCommandList(
	ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type, size_t allocatorsCount
) : m_pCommandAllocators{ allocatorsCount } {

	HRESULT hr{};
	for (auto& commandAllocator : m_pCommandAllocators) {
		D3D_THROW_FAILED(hr,
			device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator))
		);
	}

	D3D_THROW_FAILED(hr,
		device->CreateCommandList1(
			0u, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_pCommandList)
		)
	);
}

void D3DCommandList::Reset(size_t allocatorIndex) {
	ComPtr<ID3D12CommandAllocator>& currentAllocator = m_pCommandAllocators[allocatorIndex];

	HRESULT hr{};
	D3D_THROW_FAILED(hr, currentAllocator->Reset());
	D3D_THROW_FAILED(hr, m_pCommandList->Reset(currentAllocator.Get(), nullptr));
}

void D3DCommandList::Close() const {
	HRESULT hr{};
	D3D_THROW_FAILED(hr, m_pCommandList->Close());
}

ID3D12GraphicsCommandList* D3DCommandList::GetCommandList() const noexcept {
	return m_pCommandList.Get();
}
