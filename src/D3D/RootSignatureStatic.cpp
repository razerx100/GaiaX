#include <RootSignatureStatic.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dcompiler.h>

void RootSignatureStatic::LoadBinary(const std::string& fileName) {
	HRESULT hr;
	D3D_THROW_FAILED(
		hr,
		D3DReadFileToBlob(
			std::wstring(std::begin(fileName), std::end(fileName)).c_str(), &m_pSignatureBinary
		)
	);
}
