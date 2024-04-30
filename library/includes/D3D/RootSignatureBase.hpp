#ifndef ROOT_SIGNATURE_BASE_HPP_
#define ROOT_SIGNATURE_BASE_HPP_
#include <D3DHeaders.hpp>

class RootSignatureBase {
public:
	void CreateSignature(ID3D12Device* device);
	[[nodiscard]]
	ID3D12RootSignature* Get() const noexcept;

protected:
	ComPtr<ID3DBlob> m_pSignatureBinary;

private:
	ComPtr<ID3D12RootSignature> m_pRootSignature;
};
#endif
