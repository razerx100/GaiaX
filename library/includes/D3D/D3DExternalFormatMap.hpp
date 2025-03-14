#ifndef D3D_EXTERNAL_FORMAT_MAP_HPP_
#define D3D_EXTERNAL_FORMAT_MAP_HPP_
#include <D3DHeaders.hpp>
#include <ExternalFormat.hpp>

[[nodiscard]]
DXGI_FORMAT GetDxgiFormat(ExternalFormat format) noexcept;
[[nodiscard]]
ExternalFormat GetExternalFormat(DXGI_FORMAT format) noexcept;
#endif
