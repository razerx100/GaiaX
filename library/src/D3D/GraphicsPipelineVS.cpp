#include <GraphicsPipelineVS.hpp>
#include <D3DShader.hpp>
#include <VertexLayout.hpp>

// Vertex Shader
static std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineVS(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature, ShaderType binaryType,
	const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
) {
	auto vs              = std::make_unique<D3DShader>();
	const bool vsSuccess = vs->LoadBinary(
		shaderPath + graphicsExtPipeline.GetVertexShader().GetNameWithExtension(binaryType)
	);

	auto ps              = std::make_unique<D3DShader>();
	const bool fsSuccess = ps->LoadBinary(
		shaderPath + graphicsExtPipeline.GetFragmentShader().GetNameWithExtension(binaryType)
	);

	GraphicsPipelineBuilderVS builder{ graphicsRootSignature };

	builder.SetInputAssembler(
		VertexLayout{}
		.AddInputElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 12u)
		.AddInputElement("Normal", DXGI_FORMAT_R32G32B32_FLOAT, 12u)
		.AddInputElement("UV", DXGI_FORMAT_R32G32_FLOAT, 8u)
	);

	ConfigurePipelineBuilder(builder, graphicsExtPipeline);

	auto pso = std::make_unique<D3DPipelineObject>();

	if (vsSuccess && fsSuccess)
	{
		builder.SetVertexStage(vs->GetByteCode(), ps->GetByteCode());

		pso->CreateGraphicsPipeline(device, builder);
	}

	return pso;
}

void GraphicsPipelineVS::SetIATopology(const D3DCommandList& graphicsCmdList) noexcept
{
	ID3D12GraphicsCommandList* cmdList = graphicsCmdList.Get();

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// Indirect Draw
std::unique_ptr<D3DPipelineObject> GraphicsPipelineVSIndirectDraw::_createGraphicsPipeline(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsRootSignature, s_shaderBytecodeType,shaderPath, graphicsExtPipeline
	);
}

// Individual Draw
std::unique_ptr<D3DPipelineObject> GraphicsPipelineVSIndividualDraw::_createGraphicsPipeline(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsRootSignature, s_shaderBytecodeType, shaderPath, graphicsExtPipeline
	);
}
