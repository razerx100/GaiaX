#include <D3DRootSignature.hpp>

void RootSignature::CreateSignature(ID3D12Device* device, ID3DBlob* binarySignature)
{
	device->CreateRootSignature(
		0u, binarySignature->GetBufferPointer(), binarySignature->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)
	);
}
