#include <array>
#include <D3DExternalFormatMap.hpp>

static constexpr std::array s_externalFormatMap
{
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_B8G8R8A8_UNORM,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
	DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_D32_FLOAT,
	DXGI_FORMAT_D24_UNORM_S8_UINT,
	DXGI_FORMAT_D24_UNORM_S8_UINT
};

DXGI_FORMAT GetDxgiFormat(ExternalFormat format) noexcept
{
	return s_externalFormatMap[static_cast<size_t>(format)];
}

ExternalFormat GetExternalFormat(DXGI_FORMAT format) noexcept
{
	size_t formatIndex = 0u;

	for (size_t index = 0u; index < std::size(s_externalFormatMap); ++index)
		if (s_externalFormatMap[index] == format)
		{
			formatIndex = index;

			break;
		}

	return static_cast<ExternalFormat>(formatIndex);
}
