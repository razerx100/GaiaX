#include <D3DShader.hpp>
#include <d3dcompiler.h>
#include <format>
#include <Exception.hpp>
#include <fstream>

bool D3DShader::LoadBinary(const std::wstring& fileName)
{
	std::ifstream shader{ fileName.c_str(), std::ios_base::binary | std::ios_base::ate };

	bool success = false;

	if (shader.is_open())
	{
		HRESULT hr = D3DReadFileToBlob(fileName.c_str(), &m_binary);

		success    = hr == S_OK;
	}

	return success;
}

bool D3DShader::CompileBinary(
	const std::wstring& fileName, const char* target, const char* entryPoint
) {
	std::ifstream shader{ fileName.c_str(), std::ios_base::binary | std::ios_base::ate };

	bool success = false;

	if (shader.is_open())
	{
#if defined(_DEBUG)
		std::uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		std::uint32_t compileFlags = 0;
#endif

		ComPtr<ID3DBlob> error{};
		HRESULT hr = D3DCompileFromFile(
			fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target,
			compileFlags, 0u, &m_binary, &error
		);

		if (error)
		{
			std::string errorString = std::format(
				"Description : {}\n",
				reinterpret_cast<char*>(error->GetBufferPointer())
			);

			throw Exception{ "Shader Creation error", errorString };
		}

		success = hr == S_OK;
	}

	return success;
}

D3D12_SHADER_BYTECODE D3DShader::GetByteCode() const noexcept
{
	D3D12_SHADER_BYTECODE byteCode
	{
		.pShaderBytecode = m_binary->GetBufferPointer(),
		.BytecodeLength  = m_binary->GetBufferSize()
	};

	return byteCode;
}
