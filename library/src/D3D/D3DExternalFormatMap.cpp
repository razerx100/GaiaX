#include <array>
#include <D3DExternalFormatMap.hpp>

namespace Gaia
{
static constexpr std::array s_externalFormatMap
{
	DXGI_FORMAT_UNKNOWN,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_B8G8R8A8_UNORM,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
	DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_R16_FLOAT,
	DXGI_FORMAT_D32_FLOAT,
	DXGI_FORMAT_D24_UNORM_S8_UINT,
	DXGI_FORMAT_D24_UNORM_S8_UINT
};

static constexpr std::array s_blendOPMap
{
	D3D12_BLEND_OP_ADD,
	D3D12_BLEND_OP_SUBTRACT,
	D3D12_BLEND_OP_REV_SUBTRACT,
	D3D12_BLEND_OP_MIN,
	D3D12_BLEND_OP_MAX
};

static constexpr std::array s_blendFactorMap
{
	D3D12_BLEND_ONE,
	D3D12_BLEND_ZERO,
	D3D12_BLEND_SRC_COLOR,
	D3D12_BLEND_DEST_COLOR,
	D3D12_BLEND_SRC_ALPHA,
	D3D12_BLEND_DEST_ALPHA,
	D3D12_BLEND_INV_SRC_COLOR,
	D3D12_BLEND_INV_DEST_COLOR,
	D3D12_BLEND_INV_SRC_ALPHA,
	D3D12_BLEND_INV_DEST_ALPHA
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

D3D12_BLEND GetD3DBlendFactor(ExternalBlendFactor factor) noexcept
{
	return s_blendFactorMap[static_cast<size_t>(factor)];
}

D3D12_BLEND_OP GetD3DBlendOP(ExternalBlendOP op) noexcept
{
	return s_blendOPMap[static_cast<size_t>(op)];
}

D3D12_RENDER_TARGET_BLEND_DESC GetD3DBlendState(const ExternalBlendState& blendState) noexcept
{
	return D3D12_RENDER_TARGET_BLEND_DESC
	{
		.BlendEnable           = blendState.enabled ? TRUE : FALSE,
		.LogicOpEnable         = FALSE,
		.SrcBlend              = GetD3DBlendFactor(blendState.colourBlendSrc),
		.DestBlend             = GetD3DBlendFactor(blendState.colourBlendDst),
		.BlendOp               = GetD3DBlendOP(blendState.colourBlendOP),
		.SrcBlendAlpha         = GetD3DBlendFactor(blendState.alphaBlendSrc),
		.DestBlendAlpha        = GetD3DBlendFactor(blendState.alphaBlendDst),
		.BlendOpAlpha          = GetD3DBlendOP(blendState.alphaBlendOP),
		.LogicOp               = D3D12_LOGIC_OP_NOOP,
		.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
	};
}
}
