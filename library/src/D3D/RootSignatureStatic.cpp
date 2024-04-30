#include <RootSignatureStatic.hpp>
#include <d3dcompiler.h>

void RootSignatureStatic::LoadBinary(const std::string& fileName) {
	D3DReadFileToBlob(
		std::wstring(std::begin(fileName), std::end(fileName)).c_str(), &m_pSignatureBinary
	);
}
