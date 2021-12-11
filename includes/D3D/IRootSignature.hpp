#ifndef __I_ROOT_SIGNATURE_HPP__
#define __I_ROOT_SIGNATURE_HPP__
#include <D3DHeaders.hpp>

class IRootSignature {
public:
	virtual ~IRootSignature() = default;

	virtual void CreateSignature(ID3D12Device* device) = 0;
	virtual ID3D12RootSignature* Get() const noexcept = 0;
};
#endif
