#ifndef D3D_ROOT_SIGNATURE_HPP_
#define D3D_ROOT_SIGNATURE_HPP_
#include <D3DHeaders.hpp>
#include <D3DRootSignatureDynamic.hpp>
#include <utility>
#include <string>

class D3DRootSignatureStatic
{
public:
	D3DRootSignatureStatic() : m_binaryRootSignature{} {}

	void LoadBinary(const std::wstring& fileName);

	[[nodiscard]]
	ID3DBlob* GetBinary() const noexcept { return m_binaryRootSignature.Get(); }

private:
	ComPtr<ID3DBlob> m_binaryRootSignature;

public:
	D3DRootSignatureStatic(const D3DRootSignatureStatic&) = delete;
	D3DRootSignatureStatic& operator=(const D3DRootSignatureStatic&) = delete;

	D3DRootSignatureStatic(D3DRootSignatureStatic&& other) noexcept
		: m_binaryRootSignature{ std::move(other.m_binaryRootSignature) }
	{}
	D3DRootSignatureStatic& operator=(D3DRootSignatureStatic&& other) noexcept
	{
		m_binaryRootSignature = std::move(other.m_binaryRootSignature);

		return *this;
	}
};

class D3DRootSignature
{
public:
	D3DRootSignature() : m_rootSignature{} {}

	void CreateSignature(ID3D12Device* device, ID3DBlob* binarySignature);
	void CreateSignature(ID3D12Device* device, const D3DRootSignatureStatic& rsStatic)
	{
		CreateSignature(device, rsStatic.GetBinary());
	}
	void CreateSignature(ID3D12Device* device, const D3DRootSignatureDynamic& rsDynamic)
	{
		CreateSignature(device, rsDynamic.GetBinary());
	}

	[[nodiscard]]
	ID3D12RootSignature* Get() const noexcept { return m_rootSignature.Get(); }

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;

public:
	D3DRootSignature(const D3DRootSignature&) = delete;
	D3DRootSignature& operator=(const D3DRootSignature&) = delete;

	D3DRootSignature(D3DRootSignature&& other) noexcept
		: m_rootSignature{ std::move(other.m_rootSignature) }
	{}
	D3DRootSignature& operator=(D3DRootSignature&& other) noexcept
	{
		m_rootSignature = std::move(other.m_rootSignature);

		return *this;
	}
};
#endif
