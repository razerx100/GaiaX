#ifndef D3D_SHADER_HPP_
#define D3D_SHADER_HPP_
#include <D3DHeaders.hpp>
#include <string>

class D3DShader
{
public:
	void LoadBinary(const std::wstring& fileName);
	void CompileBinary(
		const std::wstring& fileName, const char* target,
		const char* entryPoint = "main"
	);

	[[nodiscard]]
	D3D12_SHADER_BYTECODE GetByteCode() const noexcept;

private:
	ComPtr<ID3DBlob> m_pBinary;
};
#endif
