module;

#include <D3DThrowMacros.hpp>

module D3DResource;

ID3D12Resource* D3DResource::Get() const noexcept {
	return m_pBuffer.Get();
}

ID3D12Resource** D3DResource::GetAddress() noexcept {
	return m_pBuffer.GetAddressOf();
}

ID3D12Resource** D3DResource::ReleaseAndGetAddress() noexcept {
	return m_pBuffer.ReleaseAndGetAddressOf();
}

void D3DResource::CreateResource(
	ID3D12Device* device, ID3D12Heap* heap, size_t offset, const D3D12_RESOURCE_DESC& desc,
	D3D12_RESOURCE_STATES initialState
) {
	HRESULT hr{};
	D3D_THROW_FAILED(hr,
		device->CreatePlacedResource(
			heap, offset, &desc, initialState, nullptr, __uuidof(ID3D12Resource), &m_pBuffer
		)
	);
}

void D3DCPUWResource::MapBuffer() {
	HRESULT hr{};
	D3D_THROW_FAILED(
		hr,
		m_pBuffer->Map(0u, nullptr, reinterpret_cast<void**>(&m_cpuHandle))
	);
}

std::uint8_t* D3DCPUWResource::GetCPUWPointer() const noexcept {
	return m_cpuHandle;
}
