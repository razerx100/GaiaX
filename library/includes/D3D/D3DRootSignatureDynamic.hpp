#ifndef D3D_ROOT_SIGNATURE_DYNAMIC_HPP_
#define D3D_ROOT_SIGNATURE_DYNAMIC_HPP_
#include <D3DHeaders.hpp>
#include <D3DDescriptorLayout.hpp>
#include <D3DResources.hpp>
#include <vector>
#include <memory>
#include <array>
#include <cassert>

enum BindlessLevel
{
	UnboundArray,
	DirectDescriptorHeapAccess,
	None
};

struct RSCompileFlagBuilder
{
	D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	RSCompileFlagBuilder& ComputeShader() noexcept
	{
		rootSigFlags |=
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

		return *this;
	}

	RSCompileFlagBuilder& MeshShader() noexcept
	{
		rootSigFlags |=
			D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

		return *this;
	}

	RSCompileFlagBuilder& VertexShader() noexcept
	{
		rootSigFlags |=
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

		return *this;
	}

	RSCompileFlagBuilder& Bindless(BindlessLevel level) noexcept
	{
		if (level == BindlessLevel::DirectDescriptorHeapAccess)
			rootSigFlags |=
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
			D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		return *this;
	}

	[[nodiscard]]
	D3D12_ROOT_SIGNATURE_FLAGS Get(BindlessLevel level) const noexcept
	{
		D3D12_ROOT_SIGNATURE_FLAGS rootSigFlagsTemp = rootSigFlags;

		if (level == BindlessLevel::DirectDescriptorHeapAccess)
			rootSigFlagsTemp |=
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
			D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		return rootSigFlagsTemp;
	}

	[[nodiscard]]
	D3D12_ROOT_SIGNATURE_FLAGS Get() const noexcept { return rootSigFlags; }
};

class D3DRootSignatureDynamic
{
	template<D3D12_ROOT_PARAMETER_TYPE RootDescriptorType>
	D3DRootSignatureDynamic& AddRootDescriptor(
		D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	) {
		assert(m_rsSizeLimit >= 2u && "Each root descriptor takes 2 Dwords. Not enough available Dwords.");

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

		m_rsSizeLimit -= 2u;

		return *this;
	}

public:
	D3DRootSignatureDynamic()
		// A root signature can have a maximum size of 64 Dwords.
		: m_binaryRootSignature{}, m_rootParameters{}, m_descriptorRanges{}, m_rsSizeLimit{ 64u } {}

	D3DRootSignatureDynamic& AddConstants(
		UINT numOfValues, D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	);

	D3DRootSignatureDynamic& AddRootCBV(
		D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	) {
		return AddRootDescriptor<D3D12_ROOT_PARAMETER_TYPE_CBV>(visibility, registerNumber, registerSpace);
	}
	D3DRootSignatureDynamic& AddRootSRV(
		D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	) {
		return AddRootDescriptor<D3D12_ROOT_PARAMETER_TYPE_SRV>(visibility, registerNumber, registerSpace);
	}
	D3DRootSignatureDynamic& AddRootUAV(
		D3D12_SHADER_VISIBILITY visibility, UINT registerNumber, UINT registerSpace = 0u
	) {
		return AddRootDescriptor<D3D12_ROOT_PARAMETER_TYPE_UAV>(visibility, registerNumber, registerSpace);
	}
	D3DRootSignatureDynamic& AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE descriptorType,
		UINT descriptorsAmount, D3D12_SHADER_VISIBILITY visibility,
		UINT baseRegister, UINT registerSpace = 0u
	);

	void CompileSignature(
		const RSCompileFlagBuilder& flagBuilder, BindlessLevel bindlessLevel,
		bool staticSampler = true, const SamplerBuilder& builder = {}
	);

	void PopulateFromLayouts(const std::vector<D3DDescriptorLayout>& layouts);

	[[nodiscard]]
	ID3DBlob* GetBinary() const noexcept { return m_binaryRootSignature.Get(); }

private:
	ComPtr<ID3DBlob>                     m_binaryRootSignature;
	std::vector<D3D12_ROOT_PARAMETER1>   m_rootParameters;
	std::vector<D3D12_DESCRIPTOR_RANGE1> m_descriptorRanges;
	UINT                                 m_rsSizeLimit;

public:
	D3DRootSignatureDynamic(const D3DRootSignatureDynamic&) = delete;
	D3DRootSignatureDynamic& operator=(const D3DRootSignatureDynamic&) = delete;

	D3DRootSignatureDynamic(D3DRootSignatureDynamic&& other) noexcept
		: m_binaryRootSignature{ std::move(other.m_binaryRootSignature) },
		m_rootParameters{ std::move(other.m_rootParameters) },
		m_descriptorRanges{ std::move(other.m_descriptorRanges) },
		m_rsSizeLimit{ other.m_rsSizeLimit }
	{}
	D3DRootSignatureDynamic& operator=(D3DRootSignatureDynamic&& other) noexcept
	{
		m_binaryRootSignature = std::move(other.m_binaryRootSignature);
		m_rootParameters      = std::move(other.m_rootParameters);
		m_descriptorRanges    = std::move(other.m_descriptorRanges);
		m_rsSizeLimit         = other.m_rsSizeLimit;

		return *this;
	}
};
#endif
