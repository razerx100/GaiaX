#ifndef D3D_EXTERNAL_FORMAT_MAP_HPP_
#define D3D_EXTERNAL_FORMAT_MAP_HPP_
#include <D3DHeaders.hpp>
#include <ExternalFormat.hpp>
#include <ExternalPipeline.hpp>

[[nodiscard]]
DXGI_FORMAT GetDxgiFormat(ExternalFormat format) noexcept;
[[nodiscard]]
ExternalFormat GetExternalFormat(DXGI_FORMAT format) noexcept;

[[nodiscard]]
D3D12_BLEND GetD3DBlendFactor(ExternalBlendFactor factor) noexcept;
[[nodiscard]]
D3D12_BLEND_OP GetD3DBlendOP(ExternalBlendOP op) noexcept;

[[nodiscard]]
D3D12_RENDER_TARGET_BLEND_DESC GetD3DBlendState(const ExternalBlendState& blendState) noexcept;
#endif
