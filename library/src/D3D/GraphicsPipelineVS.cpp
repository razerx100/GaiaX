#include <GraphicsPipelineVS.hpp>
#include <D3DShader.hpp>
#include <VertexLayout.hpp>

// Vertex Shader
static std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineVS(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature, ShaderType binaryType,
	DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat,
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

	GraphicsPipelineBuilderVS builder{ graphicsRootSignature };

	// The vertexLayout object must be alive till the pipeline is created, to preserve the
	// pointers.
	VertexLayout<3u> vertexLayout{};

	vertexLayout.AddInputElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 12u)
		.AddInputElement("Normal", DXGI_FORMAT_R32G32B32_FLOAT, 12u)
		.AddInputElement("UV", DXGI_FORMAT_R32G32_FLOAT, 8u);

	builder.SetInputAssembler(vertexLayout.GetLayoutDesc());

	builder.AddRenderTarget(rtvFormat).SetCullMode(D3D12_CULL_MODE_BACK);

	if (dsvFormat != DXGI_FORMAT_UNKNOWN)
		builder.SetDepthStencilState(
			DepthStencilStateBuilder{}.Enable(TRUE, D3D12_DEPTH_WRITE_MASK_ALL, FALSE),
			dsvFormat
		);

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
	DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat,
	const std::wstring& shaderPath, const ShaderName& pixelShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsRootSignature, s_shaderBytecodeType, rtvFormat, dsvFormat,shaderPath, pixelShader,
		L"VertexShaderIndirect"
	);
}

// Individual Draw
std::unique_ptr<D3DPipelineObject> GraphicsPipelineVSIndividualDraw::_createGraphicsPipeline(
	ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
	DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat,
	const std::wstring& shaderPath, const ShaderName& pixelShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsRootSignature, s_shaderBytecodeType, rtvFormat, dsvFormat, shaderPath, pixelShader,
		L"VertexShaderIndividual"
	);
}
