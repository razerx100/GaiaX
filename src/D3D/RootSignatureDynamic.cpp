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
	D3D12_SHADER_VISIBILITY visibility, bool bindless,
	std::uint32_t registerNumber,
	std::uint32_t registerSpace
) noexcept {
	D3D12_DESCRIPTOR_RANGE_FLAGS descFlag = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	if (descriptorType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
			descFlag = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
	else if (descriptorType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
		descFlag = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
	else if (descriptorType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
		descFlag = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

	if (bindless)
		descFlag |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

	auto descRange = std::make_unique<CD3DX12_DESCRIPTOR_RANGE1>();
	descRange->Init(
		descriptorType, descriptorsAmount, registerNumber, registerSpace, descFlag
	);

	CD3DX12_ROOT_PARAMETER1 descTableParam = {};
	descTableParam.InitAsDescriptorTable(1u, descRange.get(), visibility);

	m_rangePreserver.emplace_back(std::move(descRange));

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

void RootSignatureDynamic::AddShaderResourceView(
	D3D12_SHADER_VISIBILITY visibility,
	std::uint32_t registerNumber, std::uint32_t registerSpace
) noexcept {
	D3D12_ROOT_DESCRIPTOR_FLAGS srvFlag = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

	srvFlag = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

	CD3DX12_ROOT_PARAMETER1 srvParam{};
	srvParam.InitAsShaderResourceView(
		registerNumber, registerSpace, srvFlag, visibility
	);

	m_rootParameters.emplace_back(srvParam);
}

void RootSignatureDynamic::CompileSignature(bool staticSampler) {
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
	D3D12_ROOT_SIGNATURE_FLAGS sigFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

	D3D12_STATIC_SAMPLER_DESC staticSamplerDesc = {};

	if (staticSampler) {
		staticSamplerDesc.ShaderRegister = 0u;
		staticSamplerDesc.RegisterSpace = 0u;
		staticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		staticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

		rootSigDesc.Init_1_1(
			static_cast<std::uint32_t>(std::size(m_rootParameters)),
			std::data(m_rootParameters),
			1u,
			&staticSamplerDesc,
			sigFlags
		);
	}
	else
		rootSigDesc.Init_1_1(
			static_cast<std::uint32_t>(std::size(m_rootParameters)),
			std::data(m_rootParameters),
			0u,
			nullptr,
			sigFlags
		);

	ComPtr<ID3DBlob> error;
	D3DX12SerializeVersionedRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_1,
		&m_pSignatureBinary,
		&error
	);

	if (error)
		D3D_GENERIC_THROW(reinterpret_cast<char*>(error->GetBufferPointer()));

	m_rangePreserver = std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE1>>();
}

