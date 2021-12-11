#include <Shader.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dcompiler.h>

void Shader::LoadBinary(const std::string& fileName) {
	HRESULT hr;
	D3D_THROW_FAILED(hr, D3DReadFileToBlob(StrToWStr(fileName).c_str(), &m_pBinary));
}

void Shader::CompileBinary(
	const std::string& fileName, const char* target,
	const char* entryPoint
) {
#if defined(_DEBUG)
	std::uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	std::uint32_t compileFlags = 0;
#endif

	HRESULT hr;
	D3D_THROW_FAILED(hr, D3DCompileFromFile(
		StrToWStr(fileName).c_str(),
		nullptr,
		nullptr,
		entryPoint,
		target,
		compileFlags,
		0u,
		&m_pBinary,
		nullptr
	));
}

D3D12_SHADER_BYTECODE Shader::GetByteCode() const noexcept {
	D3D12_SHADER_BYTECODE byteCode = {};

	byteCode.BytecodeLength = m_pBinary->GetBufferSize();
	byteCode.pShaderBytecode = m_pBinary->GetBufferPointer();

	return byteCode;
}

std::wstring Shader::StrToWStr(const std::string& str) const noexcept {
	return std::wstring(str.begin(), str.end());
}

