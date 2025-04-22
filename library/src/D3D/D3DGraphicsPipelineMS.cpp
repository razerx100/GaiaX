#include <D3DGraphicsPipelineMS.hpp>
#include <D3DShader.hpp>

namespace Gaia
{
std::unique_ptr<D3DPipelineObject> GraphicsPipelineMS::_createGraphicsPipeline(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
) const {
	constexpr const wchar_t* cullingAmpShaderName   = L"MeshShaderASIndividual";
	constexpr const wchar_t* noCullingAmpShaderName = L"MeshShaderASIndividualNoCulling";

	return CreateGraphicsPipelineMS(
		device, graphicsRootSignature, s_shaderBytecodeType, shaderPath, graphicsExtPipeline,
		graphicsExtPipeline.IsGPUCullingEnabled() ? cullingAmpShaderName : noCullingAmpShaderName
	);
}

std::unique_ptr<D3DPipelineObject> GraphicsPipelineMS::CreateGraphicsPipelineMS(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature, ShaderBinaryType binaryType,
	const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline,
	const ShaderName& amplificationShader
) {
	auto ms              = std::make_unique<D3DShader>();
	const bool msSuccess = ms->LoadBinary(
		shaderPath + graphicsExtPipeline.GetVertexShader().GetNameWithExtension(binaryType)
	);

	auto as              = std::make_unique<D3DShader>();
	const bool asSuccess = as->LoadBinary(
		shaderPath + amplificationShader.GetNameWithExtension(binaryType)
	);

	auto ps              = std::make_unique<D3DShader>();
	const bool fsSuccess = ps->LoadBinary(
		shaderPath + graphicsExtPipeline.GetFragmentShader().GetNameWithExtension(binaryType)
	);

	GraphicsPipelineBuilderMS builder{ graphicsRootSignature };

	ConfigurePipelineBuilder(builder, graphicsExtPipeline);

	auto pso = std::make_unique<D3DPipelineObject>();

	if (msSuccess && fsSuccess && asSuccess)
	{
		builder.SetAmplificationStage(as->GetByteCode())
			.SetMeshStage(ms->GetByteCode(), ps->GetByteCode());
		pso->CreateGraphicsPipeline(device, builder);
	}

	return pso;
}
}
