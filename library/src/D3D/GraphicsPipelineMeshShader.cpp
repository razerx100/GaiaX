#include <GraphicsPipelineMeshShader.hpp>
#include <D3DShader.hpp>

void GraphicsPipelineMeshShader::Create(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ShaderName& pixelShader
) {
	m_pixelShader = pixelShader;

	constexpr const wchar_t* meshShaderName = L"MeshShaderIndividual";
	constexpr const wchar_t* taskShaderName = L"MeshShaderTSIndividual";

	if (m_useAmplificationShader)
		m_graphicsPipeline = CreateGraphicsPipelineMS(
			device, graphicsRootSignature, shaderPath, m_pixelShader, meshShaderName,
			taskShaderName
		);
	else
		m_graphicsPipeline = CreateGraphicsPipelineMS(
			device, graphicsRootSignature, shaderPath, m_pixelShader, meshShaderName
		);
}

std::unique_ptr<D3DPipelineObject> GraphicsPipelineMeshShader::CreateGraphicsPipelineMS(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ShaderName& pixelShader,
	const ShaderName& meshShader, const ShaderName& amplificationShader/* = {} */
) {
	auto ms              = std::make_unique<D3DShader>();
	const bool msSuccess = ms->LoadBinary(
		shaderPath + meshShader.GetNameWithExtension(s_shaderBytecodeType)
	);

	auto ps              = std::make_unique<D3DShader>();
	const bool fsSuccess = ps->LoadBinary(
		shaderPath + pixelShader.GetNameWithExtension(s_shaderBytecodeType)
	);

	GraphicsPipelineBuilderMS builder{ graphicsRootSignature };

	bool asSuccess = true;

	if (!std::empty(amplificationShader))
	{
		auto as   = std::make_unique<D3DShader>();
		asSuccess = as->LoadBinary(
			shaderPath + amplificationShader.GetNameWithExtension(s_shaderBytecodeType)
		);

		if (asSuccess)
			builder.SetAmplificationStage(as->GetByteCode());
	}

	auto pso = std::make_unique<D3DPipelineObject>();

	if (msSuccess && fsSuccess && asSuccess)
	{
		builder.SetMeshStage(ms->GetByteCode(), ps->GetByteCode());
		pso->CreateGraphicsPipeline(device, builder);
	}

	return pso;
}
