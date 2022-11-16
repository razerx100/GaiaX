#include <PipelineConstructor.hpp>
#include <VertexLayout.hpp>
#include <Shader.hpp>

std::unique_ptr<RootSignatureDynamic> CreateGraphicsRootSignature(ID3D12Device* device) {
	auto signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, D3D12_SHADER_VISIBILITY_PIXEL,
		RootSigElement::Textures, true, 1u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_VERTEX,
		RootSigElement::ModelData, false, 0u
	).AddConstants(
		1u, D3D12_SHADER_VISIBILITY_VERTEX, RootSigElement::TextureIndex, 0u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_VERTEX, RootSigElement::Camera, 1u
	).CompileSignature().CreateSignature(device);

	return signature;
}

std::unique_ptr<RootSignatureDynamic> CreateComputeRootSignature(ID3D12Device* device) {
	auto signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::ModelData, false, 0u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::IndirectArgsSRV, false, 1u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::IndirectArgsUAV, false, 2u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_ALL, RootSigElement::Camera, 0u
	).CompileSignature().CreateSignature(device);

	return signature;
}

std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineObject(
	ID3D12Device* device, const std::wstring& shaderPath,
	ID3D12RootSignature* graphicsRootSignature
) {
	auto vs = std::make_unique<Shader>();
	vs->LoadBinary(shaderPath + L"VertexShader.cso");

	auto ps = std::make_unique<Shader>();
	ps->LoadBinary(shaderPath + L"PixelShader.cso");

	auto pso = std::make_unique<D3DPipelineObject>();
	pso->CreateGFXPipelineState(
		device,
		VertexLayout()
		.AddInputElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 12u)
		.AddInputElement("UV", DXGI_FORMAT_R32G32_FLOAT, 8u),
		graphicsRootSignature, vs->GetByteCode(), ps->GetByteCode()
	);

	return pso;
}

std::unique_ptr<D3DPipelineObject> CreateComputePipelineObject(
	ID3D12Device* device, const std::wstring& shaderPath,
	ID3D12RootSignature* computeRootSignature
) {
	auto cs = std::make_unique<Shader>();
	cs->LoadBinary(shaderPath + L"ComputeShader.cso");

	auto pso = std::make_unique<D3DPipelineObject>();
	pso->CreateComputePipelineState(device, computeRootSignature, cs->GetByteCode());

	return pso;
}
