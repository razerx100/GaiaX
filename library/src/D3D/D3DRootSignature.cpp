#include <D3DRootSignature.hpp>
#include <d3dcompiler.h>

void D3DRootSignature::CreateSignature(ID3D12Device* device, ID3DBlob* binarySignature)
{
	device->CreateRootSignature(
		0u, binarySignature->GetBufferPointer(), binarySignature->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)
	);
}

// Root Signature Static
void D3DRootSignatureStatic::LoadBinary(const std::wstring& fileName)
{
	D3DReadFileToBlob(fileName.c_str(), &m_binaryRootSignature);
}
