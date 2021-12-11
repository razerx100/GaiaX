#ifndef __ROOT_SIGNATURE_STATIC_HPP__
#define __ROOT_SIGNATURE_STATIC_HPP__
#include <RootSignatureBase.hpp>
#include <string>

class RootSignatureStatic : public RootSignatureBase {
public:
	void LoadBinary(const std::string& fileName);
};
#endif
