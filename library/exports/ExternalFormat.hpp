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
	R16G16B16A16_FLOAT,
	R8_UNORM,
	R16_FLOAT,
	D32_FLOAT,
	D24_UNORM_S8_UINT,
	// Just S8 UINT isn't available on DXGI and so D24_UNORM_S8_UINT will be used.
	S8_UINT
};
#endif
