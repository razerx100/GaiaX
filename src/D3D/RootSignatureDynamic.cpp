#include <RootSignatureDynamic.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>

void RootSignatureDynamic::AddConstants(
	std::uint32_t numOfValues,
	D3D12_SHADER_VISIBILITY visibility,
	std::uint32_t registerNumber,
	std::uint32_t registerSpace
) noexcept {
	CD3DX12_ROOT_PARAMETER1 constantParam = {};
	constantParam.InitAsConstants(
		numOfValues,
		registerNumber,
		registerSpace,
		visibility
	);

	m_rootParameters.emplace_back(constantParam);
}

void RootSignatureDynamic::AddDescriptorTable(
	D3D12_DESCRIPTOR_RANGE_TYPE descriptorType,
	std::uint32_t descriptorsAmount,
	D3D12_SHADER_VISIBILITY visibility,
	std::uint32_t registerNumber,
	std::uint32_t registerSpace
) noexcept {
	CD3DX12_DESCRIPTOR_RANGE1 descRange = {};
	descRange.Init(
		descriptorType, descriptorsAmount, registerNumber, registerSpace,
		D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
	);

	// Range's address needs to be preserved till Signature Desc init
	m_rangePreserver.emplace_back(descRange);

	CD3DX12_ROOT_PARAMETER1 descTableParam = {};
	descTableParam.InitAsDescriptorTable(1, &m_rangePreserver.back(), visibility);

	m_rootParameters.emplace_back(descTableParam);
}

void RootSignatureDynamic::AddConstantBufferView(
	D3D12_SHADER_VISIBILITY visibility,
	std::uint32_t registerNumber,
	std::uint32_t registerSpace
) noexcept {
	CD3DX12_ROOT_PARAMETER1 cbvParam = {};
	cbvParam.InitAsConstantBufferView(
		registerNumber, registerSpace,
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
		visibility
	);

	m_rootParameters.emplace_back(cbvParam);
}

void RootSignatureDynamic::AddUnorderedAccessView(
	D3D12_SHADER_VISIBILITY visibility,
	std::uint32_t registerNumber,
	std::uint32_t registerSpace
) noexcept {
	CD3DX12_ROOT_PARAMETER1 uavParam = {};
	uavParam.InitAsUnorderedAccessView(
		registerNumber, registerSpace,
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
		visibility
	);

	m_rootParameters.emplace_back(uavParam);
}

void RootSignatureDynamic::CompileSignature(bool staticSampler) {
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
	D3D12_ROOT_SIGNATURE_FLAGS sigFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

	if (staticSampler) {
		D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = {};
		staticSamplerDesc.ShaderRegister = 0u;
		staticSamplerDesc.RegisterSpace = 0u;
		staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

		rootSigDesc.Init_1_1(
			static_cast<std::uint32_t>(m_rootParameters.size()),
			m_rootParameters.data(),
			1u,
			&staticSamplerDesc,
			sigFlags
		);
	}
	else
		rootSigDesc.Init_1_1(
			static_cast<std::uint32_t>(m_rootParameters.size()),
			m_rootParameters.data(),
			0u,
			nullptr,
			sigFlags
		);

	HRESULT hr;
	D3D_THROW_FAILED(hr,
		D3DX12SerializeVersionedRootSignature(
			&rootSigDesc,
			D3D_ROOT_SIGNATURE_VERSION_1_1,
			&m_pSignatureBinary,
			nullptr
		)
	);
}

