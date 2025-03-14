#ifndef EXTERNAL_FORMAT_HPP_
#define EXTERNAL_FORMAT_HPP_
#include <cstdint>

enum class ExternalFormat
{
	UNKNOWN,
	R8G8B8A8_UNORM,
	R8G8B8A8_SRGB,
	B8G8R8A8_UNORM,
	B8G8R8A8_SRGB,
	D32_FLOAT,
	D24_UNORM_S8_UINT,
	// Just S8 UINT isn't available on DXGI and so D24_UNORM_S8_UINT will be used.
	S8_UINT
};
#endif
