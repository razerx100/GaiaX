#ifndef D3D_ROOT_SIGNATURE_DYNAMIC_HPP_
#define D3D_ROOT_SIGNATURE_DYNAMIC_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <memory>
#include <array>

// Probably should move this to somewhere better at some point.
class SamplerBuilder
{
public:
	SamplerBuilder();

	SamplerBuilder& Anisotropy(UINT anisotropy) noexcept
	{
		m_samplerDesc.MaxAnisotropy = anisotropy;

		return *this;
	}
	SamplerBuilder& Filter(D3D12_FILTER filter) noexcept
	{
		m_samplerDesc.Filter = filter;

		return *this;
	}
	SamplerBuilder& AddressU(D3D12_TEXTURE_ADDRESS_MODE mode) noexcept
	{
		m_samplerDesc.AddressU = mode;

		return *this;
	}
	SamplerBuilder& AddressV(D3D12_TEXTURE_ADDRESS_MODE mode) noexcept
	{
		m_samplerDesc.AddressV = mode;

		return *this;
	}
	SamplerBuilder& AddressW(D3D12_TEXTURE_ADDRESS_MODE mode) noexcept
	{
		m_samplerDesc.AddressW = mode;

		return *this;
	}
	SamplerBuilder& CompareOp(D3D12_COMPARISON_FUNC compare) noexcept
	{
		m_samplerDesc.ComparisonFunc = compare;

		return *this;
	}
	SamplerBuilder& MipLOD(FLOAT value) noexcept
	{
		m_samplerDesc.MipLODBias = value;

		return *this;
	}
	SamplerBuilder& MinLOD(FLOAT value) noexcept
	{
		m_samplerDesc.MinLOD = value;

		return *this;
	}
	SamplerBuilder& MaxLOD(FLOAT value) noexcept
	{
		m_samplerDesc.MaxLOD = value;

		return *this;
	}
	SamplerBuilder& BorderColour(D3D12_STATIC_BORDER_COLOR colour) noexcept
	{
		m_samplerDesc.BorderColor = colour;
		m_borderColour            = ConvertColour(colour);

		return *this;
	}
	SamplerBuilder& BorderColour(std::array<FLOAT, 4u> colour) noexcept
	{
		m_borderColour            = colour;
		m_samplerDesc.BorderColor = ConvertColour(colour);

		return *this;
	}

	SamplerBuilder& RegisterSpace(
		UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visiblity
	) noexcept {
		m_samplerDesc.ShaderRegister   = shaderRegister;
		m_samplerDesc.RegisterSpace    = registerSpace;
		m_samplerDesc.ShaderVisibility = visiblity;

		return *this;
	}

	[[nodiscard]]
	D3D12_STATIC_SAMPLER_DESC GetStatic() const noexcept
	{
		return m_samplerDesc;
	}
	[[nodiscard]]
	const D3D12_STATIC_SAMPLER_DESC* GetStaticPtr() const noexcept
	{
		return &m_samplerDesc;
	}

	[[nodiscard]]
	D3D12_SAMPLER_DESC Get() const noexcept
	{
		return GetSamplerDesc(m_samplerDesc, m_borderColour);
	}

private:
	[[nodiscard]]
	static std::array<FLOAT, 4u> ConvertColour(D3D12_STATIC_BORDER_COLOR colour) noexcept;
	[[nodiscard]]
	static D3D12_STATIC_BORDER_COLOR ConvertColour(const std::array<FLOAT, 4u>& colour) noexcept;

	static D3D12_SAMPLER_DESC GetSamplerDesc(
		const D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc, const std::array<FLOAT, 4u>& borderColour
	) noexcept;

private:
	D3D12_STATIC_SAMPLER_DESC m_samplerDesc;
	std::array<FLOAT, 4u>     m_borderColour;
};

class D3DRootSignatureDynamic
{
	template<D3D12_ROOT_PARAMETER_TYPE RootDescriptorType>
	D3DRootSignatureDynamic& AddRootDescriptor(
		D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	) noexcept {
		m_rootParameters.emplace_back(D3D12_ROOT_PARAMETER1
			{
				.ParameterType    = RootDescriptorType,
				.Descriptor       = D3D12_ROOT_DESCRIPTOR1
				{
					.ShaderRegister = registerNumber,
					.RegisterSpace  = registerSpace,
					.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE
				},
				.ShaderVisibility = visibility
			});

		return *this;
	}

public:
	D3DRootSignatureDynamic() : m_binaryRootSignature{}, m_rootParameters{}, m_descriptorRanges{} {}

	[[nodiscard]]
	D3DRootSignatureDynamic& AddConstants(
		UINT numOfValues, D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	) noexcept;

	[[nodiscard]]
	D3DRootSignatureDynamic& AddRootCBV(
		D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	) noexcept {
		return AddRootDescriptor<D3D12_ROOT_PARAMETER_TYPE_CBV>(visibility, registerNumber, registerSpace);
	}
	[[nodiscard]]
	D3DRootSignatureDynamic& AddRootSRV(
		D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	) noexcept {
		return AddRootDescriptor<D3D12_ROOT_PARAMETER_TYPE_SRV>(visibility, registerNumber, registerSpace);
	}
	[[nodiscard]]
	D3DRootSignatureDynamic& AddRootUAV(
		D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	) noexcept {
		return AddRootDescriptor<D3D12_ROOT_PARAMETER_TYPE_UAV>(visibility, registerNumber, registerSpace);
	}
	[[nodiscard]]
	D3DRootSignatureDynamic& AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE descriptorType,
		UINT descriptorsAmount, D3D12_SHADER_VISIBILITY visibility,
		bool bindless, UINT baseRegister, UINT registerSpace = 0u
	) noexcept;

	void CompileSignature(
		bool meshShader,
		bool staticSampler = true, const SamplerBuilder& builder = {}
	);

	[[nodiscard]]
	ID3DBlob* GetBinary() const noexcept { return m_binaryRootSignature.Get(); }

private:
	ComPtr<ID3DBlob>                     m_binaryRootSignature;
	std::vector<D3D12_ROOT_PARAMETER1>   m_rootParameters;
	std::vector<D3D12_DESCRIPTOR_RANGE1> m_descriptorRanges;

public:
	D3DRootSignatureDynamic(const D3DRootSignatureDynamic&) = delete;
	D3DRootSignatureDynamic& operator=(const D3DRootSignatureDynamic&) = delete;

	D3DRootSignatureDynamic(D3DRootSignatureDynamic&& other) noexcept
		: m_binaryRootSignature{ std::move(other.m_binaryRootSignature) },
		m_rootParameters{ std::move(other.m_rootParameters) },
		m_descriptorRanges{ std::move(other.m_descriptorRanges) }
	{}
	D3DRootSignatureDynamic& operator=(D3DRootSignatureDynamic&& other) noexcept
	{
		m_binaryRootSignature = std::move(other.m_binaryRootSignature);
		m_rootParameters      = std::move(other.m_rootParameters);
		m_descriptorRanges    = std::move(other.m_descriptorRanges);

		return *this;
	}
};
#endif
