#include <Shader.hpp>
#include <d3dcompiler.h>
#include <fstream>
#include <cassert>

void Shader::LoadBinary(const std::wstring& fileName) {
	D3DReadFileToBlob(fileName.c_str(), &m_pBinary);
}

void Shader::CompileBinary(
	const std::wstring& fileName, const char* target,
	const char* entryPoint
) {
#if defined(_DEBUG)
	std::uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	std::uint32_t compileFlags = 0;
#endif

	ComPtr<ID3DBlob> error;
	D3DCompileFromFile(
		fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target,
		compileFlags, 0u, &m_pBinary, &error
	);

	if (error) {
		std::ofstream log("ErrorLog.txt", std::ios_base::app | std::ios_base::out);
		log << "Category : Shader Creation error    "
			<< "Description : " << reinterpret_cast<char*>(error->GetBufferPointer()) << "    "
			<< std::endl;
	}

	assert(!error && "Shader Creation error.");
}

D3D12_SHADER_BYTECODE Shader::GetByteCode() const noexcept {
	D3D12_SHADER_BYTECODE byteCode{};

	byteCode.BytecodeLength = m_pBinary->GetBufferSize();
	byteCode.pShaderBytecode = m_pBinary->GetBufferPointer();

	return byteCode;
}
