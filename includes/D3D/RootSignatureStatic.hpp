#ifndef ROOT_SIGNATURE_STATIC_HPP_
#define ROOT_SIGNATURE_STATIC_HPP_
#include <RootSignatureBase.hpp>
#include <string>

class RootSignatureStatic : public RootSignatureBase {
public:
	void LoadBinary(const std::string& fileName);
};
#endif
