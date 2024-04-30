#include <RootSignatureBase.hpp>

void RootSignatureBase::CreateSignature(ID3D12Device* device) {
	device->CreateRootSignature(
		0u, m_pSignatureBinary->GetBufferPointer(), m_pSignatureBinary->GetBufferSize(),
		IID_PPV_ARGS(&m_pRootSignature)
	);
}

ID3D12RootSignature* RootSignatureBase::Get() const noexcept {
	return m_pRootSignature.Get();
}
