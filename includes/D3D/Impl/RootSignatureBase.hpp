#ifndef __ROOT_SIGNATURE_BASE_HPP__
#define __ROOT_SIGNATURE_BASE_HPP__
#include <IRootSignature.hpp>

class RootSignatureBase : public IRootSignature {
public:
	void CreateSignature(ID3D12Device* device) override;
	ID3D12RootSignature* Get() const noexcept override;

protected:
	ComPtr<ID3DBlob> m_pSignatureBinary;

private:
	ComPtr<ID3D12RootSignature> m_pRootSignature;
};
#endif
