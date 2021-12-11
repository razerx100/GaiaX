#include <RootSignatureBase.hpp>
#include <D3DThrowMacros.hpp>

void RootSignatureBase::CreateSignature(ID3D12Device* device) {
	HRESULT hr;
	D3D_THROW_FAILED(
		hr, device->CreateRootSignature(
			0u,
			m_pSignatureBinary->GetBufferPointer(),
			m_pSignatureBinary->GetBufferSize(),
			__uuidof(ID3D12RootSignature),
			&m_pRootSignature
		)
	);
}

ID3D12RootSignature* RootSignatureBase::Get() const noexcept {
	return m_pRootSignature.Get();
}
