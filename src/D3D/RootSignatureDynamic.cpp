#include <RootSignatureDynamic.hpp>
#include <d3dx12.h>
#include <fstream>
#include <cassert>

RootSignatureDynamic::RootSignatureDynamic() noexcept :
	m_elementLayout(static_cast<size_t>(RootSigElement::ElementCount)) {}

RootSignatureDynamic& RootSignatureDynamic::AddConstants(
	std::uint32_t numOfValues, D3D12_SHADER_VISIBILITY visibility, RootSigElement elementType,
	std::uint32_t registerNumber, std::uint32_t registerSpace
) noexcept {
	AddElementType(elementType);

	CD3DX12_ROOT_PARAMETER1 constantParam{};
	constantParam.InitAsConstants(
		numOfValues,
		registerNumber,
		registerSpace,
		visibility
	);

	m_rootParameters.emplace_back(constantParam);

	return *this;
}

RootSignatureDynamic& RootSignatureDynamic::AddDescriptorTable(
	D3D12_DESCRIPTOR_RANGE_TYPE descriptorType, std::uint32_t descriptorsAmount,
	D3D12_SHADER_VISIBILITY visibility, RootSigElement elementType, bool bindless,
	std::uint32_t registerNumber, std::uint32_t registerSpace
) noexcept {
	AddElementType(elementType);

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

	CD3DX12_ROOT_PARAMETER1 descTableParam{};
	descTableParam.InitAsDescriptorTable(1u, descRange.get(), visibility);

	m_rangePreserver.emplace_back(std::move(descRange));
	m_rootParameters.emplace_back(descTableParam);

	return *this;
}

RootSignatureDynamic& RootSignatureDynamic::AddConstantBufferView(
	D3D12_SHADER_VISIBILITY visibility, RootSigElement elementType,
	std::uint32_t registerNumber, std::uint32_t registerSpace
) noexcept {
	AddElementType(elementType);

	CD3DX12_ROOT_PARAMETER1 cbvParam{};
	cbvParam.InitAsConstantBufferView(
		registerNumber, registerSpace,
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE,
		visibility
	);

	m_rootParameters.emplace_back(cbvParam);

	return *this;
}

RootSignatureDynamic& RootSignatureDynamic::AddUnorderedAccessView(
	D3D12_SHADER_VISIBILITY visibility, RootSigElement elementType,
	std::uint32_t registerNumber, std::uint32_t registerSpace
) noexcept {
	AddElementType(elementType);

	CD3DX12_ROOT_PARAMETER1 uavParam{};
	uavParam.InitAsUnorderedAccessView(
		registerNumber, registerSpace,
		D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
		visibility
	);

	m_rootParameters.emplace_back(uavParam);

	return *this;
}

RootSignatureDynamic& RootSignatureDynamic::AddShaderResourceView(
	D3D12_SHADER_VISIBILITY visibility, RootSigElement elementType,
	std::uint32_t registerNumber, std::uint32_t registerSpace
) noexcept {
	AddElementType(elementType);

	D3D12_ROOT_DESCRIPTOR_FLAGS srvFlag = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

	srvFlag = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

	CD3DX12_ROOT_PARAMETER1 srvParam{};
	srvParam.InitAsShaderResourceView(
		registerNumber, registerSpace, srvFlag, visibility
	);

	m_rootParameters.emplace_back(srvParam);

	return *this;
}

RootSignatureDynamic& RootSignatureDynamic::CompileSignature(bool staticSampler) {
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc{};
	D3D12_ROOT_SIGNATURE_FLAGS sigFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

	D3D12_STATIC_SAMPLER_DESC staticSamplerDesc{};

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

	if (error) {
		std::ofstream log("ErrorLog.txt", std::ios_base::app | std::ios_base::out);
		log << "Category : Root Signature Creation error    "
			<< "Description : " << reinterpret_cast<char*>(error->GetBufferPointer()) << "    "
			<< std::endl;
	}

	assert(!error && "Root Signature Creation error.");

	m_rangePreserver = std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE1>>();

	return *this;
}

void RootSignatureDynamic::AddElementType(RootSigElement elementType) noexcept {
	assert(elementType != RootSigElement::ElementCount && "Invalid Root Element Type");
	m_elementLayout[static_cast<size_t>(elementType)] =
		static_cast<UINT>(std::size(m_rootParameters));
}

RSLayoutType RootSignatureDynamic::GetElementLayout() const noexcept {
	return m_elementLayout;
}
