#include <D3DRootSignatureDynamic.hpp>
#include <fstream>
#include <Exception.hpp>
#include <format>

// Sampler Builder
SamplerBuilder::SamplerBuilder()
	: m_samplerDesc{
		.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		.MipLODBias       = 0.f,
		.MaxAnisotropy    = 1u,
		.ComparisonFunc   = D3D12_COMPARISON_FUNC_ALWAYS,
		.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
		.MinLOD           = 0.f,
		.MaxLOD           = 0.f,
		.ShaderRegister   = 0u,
		.RegisterSpace    = 0u,
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
	},
	m_borderColour{ 0.f, 0.f, 0.f, 1.f }
{}

std::array<FLOAT, 4u> SamplerBuilder::ConvertColour(D3D12_STATIC_BORDER_COLOR colour) noexcept
{
	std::array<FLOAT, 4u> outputColour{ 0.f, 0.f, 0.f, 1.f };

	if (colour == D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK)
		outputColour = { 0.f, 0.f, 0.f, 0.1f };
	else if (colour == D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE)
		outputColour = { 1.f, 1.f, 1.f, 1.f };

	return outputColour;
}

D3D12_STATIC_BORDER_COLOR SamplerBuilder::ConvertColour(const std::array<FLOAT, 4u>& colour) noexcept
{
	D3D12_STATIC_BORDER_COLOR borderColour = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	std::array<int, 4u> intColour{};

	intColour[0] = static_cast<int>(colour[0]);
	intColour[1] = static_cast<int>(colour[1]);
	intColour[2] = static_cast<int>(colour[2]);
	intColour[3] = static_cast<int>(colour[3]);

	if (intColour[0] == 0 && intColour[1] == 0 && intColour[2] == 0 && intColour[3] == 0)
		borderColour = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	else if (intColour[0] == 1 && intColour[1] == 1 && intColour[2] == 1 && intColour[3] == 1)
		borderColour = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;

	return borderColour;
}

D3D12_SAMPLER_DESC SamplerBuilder::GetSamplerDesc(
	const D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc, const std::array<FLOAT, 4u>& borderColour
) noexcept {
	D3D12_SAMPLER_DESC samplerDesc{
		.Filter           = staticSamplerDesc.Filter,
		.AddressU         = staticSamplerDesc.AddressU,
		.AddressV         = staticSamplerDesc.AddressV,
		.AddressW         = staticSamplerDesc.AddressW,
		.MipLODBias       = staticSamplerDesc.MipLODBias,
		.MaxAnisotropy    = staticSamplerDesc.MaxAnisotropy,
		.ComparisonFunc   = staticSamplerDesc.ComparisonFunc,
		.MinLOD           = staticSamplerDesc.MinLOD,
		.MaxLOD           = staticSamplerDesc.MaxLOD
	};

	samplerDesc.BorderColor[0] = borderColour[0];
	samplerDesc.BorderColor[1] = borderColour[1];
	samplerDesc.BorderColor[2] = borderColour[2];
	samplerDesc.BorderColor[3] = borderColour[3];

	return samplerDesc;
}

// D3DRootSignatureDynamic
void D3DRootSignatureDynamic::CompileSignature(
	bool meshShader,
	bool staticSampler/* = true */, const SamplerBuilder& builder/* = {} */
) {
	// Should be fine to assign the pointers now, as there wouldn't be any new entries to
	// the Ranges vector.
	for (size_t index = 0u, rangeIndex = 0u; index < std::size(m_rootParameters); ++index)
		if (m_rootParameters[index].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			m_rootParameters[index].DescriptorTable.pDescriptorRanges = &m_descriptorRanges[rangeIndex];

			++rangeIndex;
		}

	D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	if (!meshShader)
		rootSigFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc
	{
		.Version  = D3D_ROOT_SIGNATURE_VERSION_1_1,
		.Desc_1_1 = D3D12_ROOT_SIGNATURE_DESC1
		{
			.NumParameters = static_cast<UINT>(std::size(m_rootParameters)),
			.pParameters   = std::data(m_rootParameters),
			.Flags         = rootSigFlags
		}
	};

	if (staticSampler)
	{
		D3D12_ROOT_SIGNATURE_DESC1& rootSigDesc1 = rootSigDesc.Desc_1_1;
		rootSigDesc1.NumStaticSamplers           = 1u;
		rootSigDesc1.pStaticSamplers             = builder.GetStaticPtr();
	}

	ComPtr<ID3DBlob> error{};

	D3D12SerializeVersionedRootSignature(&rootSigDesc, &m_binaryRootSignature, &error);

	if (error)
	{
		std::string errorString = std::format(
			"Description : {}\n",
			reinterpret_cast<char*>(error->GetBufferPointer())
		);

		throw Exception{ "Root Signature Creation Error", errorString };
	}
}

D3DRootSignatureDynamic& D3DRootSignatureDynamic::AddConstants(
	UINT numOfValues, D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace/* = 0u */
) noexcept {
	m_rootParameters.emplace_back(D3D12_ROOT_PARAMETER1
	{
		.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
		.Constants        = D3D12_ROOT_CONSTANTS
		{
			.ShaderRegister = registerNumber,
			.RegisterSpace  = registerSpace,
			.Num32BitValues = numOfValues
		},
		.ShaderVisibility = visibility
	});

	return *this;
}

D3DRootSignatureDynamic& D3DRootSignatureDynamic::AddDescriptorTable(
	D3D12_DESCRIPTOR_RANGE_TYPE descriptorType,
	UINT descriptorsAmount, D3D12_SHADER_VISIBILITY visibility,
	bool bindless, UINT baseRegister, UINT registerSpace /* = 0u */
) noexcept {
	D3D12_DESCRIPTOR_RANGE_FLAGS rangeFlag = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	if (bindless)
		rangeFlag |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

	m_descriptorRanges.emplace_back(D3D12_DESCRIPTOR_RANGE1
	{
		.RangeType                         = descriptorType,
		.NumDescriptors                    = descriptorsAmount,
		.BaseShaderRegister                = baseRegister,
		.RegisterSpace                     = registerSpace,
		.Flags                             = rangeFlag,
		.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	});

	m_rootParameters.emplace_back(D3D12_ROOT_PARAMETER1
	{
		.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
		.DescriptorTable  = D3D12_ROOT_DESCRIPTOR_TABLE1
		{
			.NumDescriptorRanges = 1u,
			// The pointers are assigned before creation so the latest pointer is used.
			.pDescriptorRanges   = nullptr
		},
		.ShaderVisibility = visibility
	});

	return *this;
}