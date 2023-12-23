#include <D3DCommandList.hpp>
#include <Exception.hpp>

D3DCommandList::D3DCommandList(
		ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type, bool cmdList6, size_t allocatorCount
) : m_pCommandAllocators{ allocatorCount }
{
	for (auto& commandAllocator : m_pCommandAllocators)
		device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));

	device->CreateCommandList1(
		0u, type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_pCommandList)
	);

	if (cmdList6)
	{
		HRESULT hr = m_pCommandList.As(&m_pCommandList6);

		if (hr == E_NOINTERFACE)
			throw Exception("Interface Query Error", "CommandList6 is not supported.");
	}
}

void D3DCommandList::Reset(size_t allocatorIndex) {
	ComPtr<ID3D12CommandAllocator>& currentAllocator = m_pCommandAllocators[allocatorIndex];

	currentAllocator->Reset();
	m_pCommandList->Reset(currentAllocator.Get(), nullptr);
}

void D3DCommandList::ResetFirst() {
	Reset(0u);
}

void D3DCommandList::Close() const {
	m_pCommandList->Close();
}

ID3D12GraphicsCommandList* D3DCommandList::GetCommandList() const noexcept {
	return m_pCommandList.Get();
}

ID3D12GraphicsCommandList6* D3DCommandList::GetCommandList6() const noexcept {
	return m_pCommandList6.Get();
}
