#include <GraphicsPipelineVertexShader.hpp>
#include <D3DShader.hpp>
#include <VertexLayout.hpp>

// Vertex Shader
std::unique_ptr<D3DPipelineObject> GraphicsPipelineVertexShader::CreateGraphicsPipelineVS(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature, ShaderType binaryType,
	const std::wstring& shaderPath, const ShaderName& pixelShader, const ShaderName& vertexShader
) {
	auto vs              = std::make_unique<D3DShader>();
	const bool vsSuccess = vs->LoadBinary(
		shaderPath + vertexShader.GetNameWithExtension(binaryType)
	);

	auto ps              = std::make_unique<D3DShader>();
	const bool fsSuccess = ps->LoadBinary(
		shaderPath + pixelShader.GetNameWithExtension(binaryType)
	);

	auto pso = std::make_unique<D3DPipelineObject>();

	if (vsSuccess && fsSuccess)
		pso->CreateGraphicsPipeline(
			device,
			GraphicsPipelineBuilderVS{ graphicsRootSignature }
			.SetInputAssembler(
				VertexLayout{}
				.AddInputElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 12u)
				.AddInputElement("Normal",   DXGI_FORMAT_R32G32B32_FLOAT, 12u)
				.AddInputElement("UV",       DXGI_FORMAT_R32G32_FLOAT,    8u)
				.GetLayoutDesc()
			)
			.SetVertexStage(vs->GetByteCode(), ps->GetByteCode())
		);

	return pso;
}

void GraphicsPipelineVertexShader::SetIATopology(const D3DCommandList& graphicsCmdList) noexcept
{
	ID3D12GraphicsCommandList* cmdList = graphicsCmdList.Get();

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// Indirect Draw
std::unique_ptr<D3DPipelineObject> GraphicsPipelineIndirectDraw::_createGraphicsPipeline(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ShaderName& pixelShader
) const {
	return GraphicsPipelineVertexShader::CreateGraphicsPipelineVS(
		device, graphicsRootSignature, s_shaderBytecodeType,shaderPath, pixelShader,
		L"VertexShaderIndirect"
	);
}

// Individual Draw
std::unique_ptr<D3DPipelineObject> GraphicsPipelineIndividualDraw::_createGraphicsPipeline(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ShaderName& pixelShader
) const {
	return GraphicsPipelineVertexShader::CreateGraphicsPipelineVS(
		device, graphicsRootSignature, s_shaderBytecodeType, shaderPath, pixelShader,
		L"VertexShaderIndividual"
	);
}
