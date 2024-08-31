#ifndef D3D_ROOT_SIGNATURE_HPP_
#define D3D_ROOT_SIGNATURE_HPP_
#include <D3DHeaders.hpp>
#include <utility>

class RootSignature
{
public:
	RootSignature() : m_rootSignature{} {}

	void CreateSignature(ID3D12Device* device, ID3DBlob* binarySignature);

	[[nodiscard]]
	ID3D12RootSignature* Get() const noexcept { return m_rootSignature.Get(); }

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;

public:
	RootSignature(const RootSignature&) = delete;
	RootSignature& operator=(const RootSignature&) = delete;

	RootSignature(RootSignature&& other) noexcept
		: m_rootSignature{ std::move(other.m_rootSignature) }
	{}
	RootSignature& operator=(RootSignature&& other) noexcept
	{
		m_rootSignature = std::move(other.m_rootSignature);

		return *this;
	}
};
#endif
