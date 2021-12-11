#ifndef __I_SHADER_HPP__
#define __I_SHADER_HPP__
#include <D3DHeaders.hpp>
#include <string>

class IShader {
public:
	virtual ~IShader() = default;

	virtual void LoadBinary(const std::string& fileName) = 0;
	virtual void CompileBinary(
		const std::string& fileName, const char* target,
		const char* entryPoint = "main"
	) = 0;

	virtual D3D12_SHADER_BYTECODE GetByteCode() const noexcept = 0;
};
#endif
