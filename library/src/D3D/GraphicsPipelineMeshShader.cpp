#include <GraphicsPipelineMeshShader.hpp>
#include <D3DShader.hpp>

std::unique_ptr<D3DPipelineObject> GraphicsPipelineMeshShader::_createGraphicsPipeline(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ShaderName& pixelShader
) const {
	constexpr const wchar_t* meshShaderName = L"MeshShaderMSIndividual";
	constexpr const wchar_t* ampShaderName  = L"MeshShaderASIndividual";

	return CreateGraphicsPipelineMS(
		device, graphicsRootSignature, s_shaderBytecodeType, shaderPath, pixelShader, meshShaderName,
		ampShaderName
	);
}

std::unique_ptr<D3DPipelineObject> GraphicsPipelineMeshShader::CreateGraphicsPipelineMS(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	ShaderType binaryType, const std::wstring& shaderPath, const ShaderName& pixelShader,
	const ShaderName& meshShader, const ShaderName& amplificationShader
) {
	auto ms              = std::make_unique<D3DShader>();
	const bool msSuccess = ms->LoadBinary(
		shaderPath + meshShader.GetNameWithExtension(binaryType)
	);

	auto as              = std::make_unique<D3DShader>();
	const bool asSuccess = as->LoadBinary(
		shaderPath + amplificationShader.GetNameWithExtension(binaryType)
	);

	auto ps              = std::make_unique<D3DShader>();
	const bool fsSuccess = ps->LoadBinary(
		shaderPath + pixelShader.GetNameWithExtension(binaryType)
	);

	GraphicsPipelineBuilderMS builder{ graphicsRootSignature };

	auto pso = std::make_unique<D3DPipelineObject>();

	if (msSuccess && fsSuccess && asSuccess)
	{
		builder.SetAmplificationStage(as->GetByteCode())
			.SetMeshStage(ms->GetByteCode(), ps->GetByteCode());
		pso->CreateGraphicsPipeline(device, builder);
	}

	return pso;
}
