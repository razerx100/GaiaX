#include <GraphicsPipelineVertexShader.hpp>
#include <D3DShader.hpp>
#include <VertexLayout.hpp>

// Vertex Shader
std::unique_ptr<D3DPipelineObject> GraphicsPipelineVertexShader::CreateGraphicsPipelineVS(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ShaderName& pixelShader, const ShaderName& vertexShader
) {
	auto vs              = std::make_unique<D3DShader>();
	const bool vsSuccess = vs->LoadBinary(
		shaderPath + vertexShader.GetNameWithExtension(s_shaderBytecodeType)
	);

	auto ps              = std::make_unique<D3DShader>();
	const bool fsSuccess = ps->LoadBinary(
		shaderPath + pixelShader.GetNameWithExtension(s_shaderBytecodeType)
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

void GraphicsPipelineVertexShader::Create(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ShaderName& pixelShader
) {
	m_pixelShader = pixelShader;

	m_graphicsPipeline = _createGraphicsPipeline(
		device, graphicsRootSignature, shaderPath, m_pixelShader
	);
}

// Indirect Draw
std::unique_ptr<D3DPipelineObject> GraphicsPipelineIndirectDraw::_createGraphicsPipeline(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ShaderName& pixelShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsRootSignature, shaderPath, pixelShader, L"VertexShaderIndirect"
	);
}

// Individual Draw
std::unique_ptr<D3DPipelineObject> GraphicsPipelineIndividualDraw::_createGraphicsPipeline(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath, const ShaderName& pixelShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsRootSignature, shaderPath, pixelShader, L"VertexShaderIndividual"
	);
}
