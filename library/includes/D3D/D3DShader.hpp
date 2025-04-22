#ifndef D3D_SHADER_HPP_
#define D3D_SHADER_HPP_
#include <D3DHeaders.hpp>
#include <string>

namespace Gaia
{
class D3DShader
{
public:
	D3DShader() : m_binary{} {}

	[[nodiscard]]
	bool LoadBinary(const std::wstring& fileName);
	[[nodiscard]]
	bool CompileBinary(
		const std::wstring& fileName, const char* target, const char* entryPoint = "main"
	);

	[[nodiscard]]
	D3D12_SHADER_BYTECODE GetByteCode() const noexcept;

private:
	ComPtr<ID3DBlob> m_binary;

public:
	D3DShader(const D3DShader&) = delete;
	D3DShader& operator=(const D3DShader&) = delete;

	D3DShader(D3DShader&& other) noexcept
		: m_binary{ std::move(other.m_binary) }
	{}
	D3DShader& operator=(D3DShader&& other) noexcept
	{
		m_binary = std::move(other.m_binary);

		return *this;
	}
};
}
#endif
