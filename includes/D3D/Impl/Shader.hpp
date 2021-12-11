#ifndef __SHADER_HPP__
#define __SHADER_HPP__
#include <IShader.hpp>
#include <string>

class Shader : public IShader {
public:
	void LoadBinary(const std::string& fileName) override;
	void CompileBinary(
		const std::string& fileName, const char* target,
		const char* entryPoint = "main"
	) override;

	D3D12_SHADER_BYTECODE GetByteCode() const noexcept override;

private:
	std::wstring StrToWStr(const std::string& str) const noexcept;

private:
	ComPtr<ID3DBlob> m_pBinary;
};
#endif
