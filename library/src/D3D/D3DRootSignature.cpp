#include <D3DRootSignature.hpp>
#include <d3dcompiler.h>

void D3DRootSignature::CreateSignature(ID3D12Device* device, ID3DBlob* binarySignature)
{
	device->CreateRootSignature(
		0u, binarySignature->GetBufferPointer(), binarySignature->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)
	);
}

void D3DRootSignature::BindToGraphics(const D3DCommandList& commandList) const
{
	ID3D12GraphicsCommandList* cmdList = commandList.Get();

	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
}

void D3DRootSignature::BindToCompute(const D3DCommandList& commandList) const
{
	ID3D12GraphicsCommandList* cmdList = commandList.Get();

	cmdList->SetComputeRootSignature(m_rootSignature.Get());
}

// Root Signature Static
void D3DRootSignatureStatic::LoadBinary(const std::wstring& fileName)
{
	D3DReadFileToBlob(fileName.c_str(), &m_binaryRootSignature);
}
