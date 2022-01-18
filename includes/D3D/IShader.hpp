#ifndef __I_SHADER_HPP__
#define __I_SHADER_HPP__
#include <D3DHeaders.hpp>

class IShader {
public:
	virtual ~IShader() = default;

	virtual D3D12_SHADER_BYTECODE GetByteCode() const noexcept = 0;
};
#endif
