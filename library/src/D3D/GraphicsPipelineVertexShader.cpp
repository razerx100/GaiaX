#include <GraphicsPipelineVertexShader.hpp>
#include <VertexLayout.hpp>
#include <D3DShader.hpp>

// Vertex Shader
std::unique_ptr<D3DPipelineObject> GraphicsPipelineVertexShader::CreateGraphicsPipelineObjectVS(
	ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
	const std::wstring& vertexShader, ID3D12RootSignature* graphicsRootSignature
) const noexcept {
	auto vs = std::make_unique<D3DShader>();
	vs->LoadBinary(shaderPath + vertexShader);

	auto ps = std::make_unique<D3DShader>();
	ps->LoadBinary(shaderPath + pixelShader);

	auto pso = std::make_unique<D3DPipelineObject>();
	pso->CreateGFXPipelineStateVertex(
		device,
		VertexLayout()
		.AddInputElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 12u)
		.AddInputElement("Normal", DXGI_FORMAT_R32G32B32_FLOAT, 12u)
		.AddInputElement("UV", DXGI_FORMAT_R32G32_FLOAT, 8u).GetLayoutDesc(),
		graphicsRootSignature, vs->GetByteCode(), ps->GetByteCode()
	);

	return pso;
}

// Indirect Draw
GraphicsPipelineIndirectDraw::GraphicsPipelineIndirectDraw() noexcept
	: m_modelCount{ 0u }, m_counterBufferOffset{ 0u }, m_argumentBufferOffset{ 0u } {}

void GraphicsPipelineIndirectDraw::DrawModels(
	ID3D12CommandSignature* commandSignature, ID3D12GraphicsCommandList* graphicsCommandList,
	ID3D12Resource* argumentBuffer, ID3D12Resource* counterBuffer
) const noexcept {
	graphicsCommandList->ExecuteIndirect(
		commandSignature, m_modelCount, argumentBuffer, m_argumentBufferOffset,
		counterBuffer, m_counterBufferOffset
	);
}

void GraphicsPipelineIndirectDraw::ConfigureGraphicsPipelineObject(
	const std::wstring& pixelShader, UINT modelCount, std::uint32_t modelCountOffset,
	size_t counterIndex
) noexcept {
	m_modelCount = modelCount;

	m_argumentBufferOffset = sizeof(ModelDrawArguments) * modelCountOffset;
	m_counterBufferOffset = sizeof(std::uint32_t) * 2u * counterIndex;
	m_pixelShader = pixelShader;
}

std::unique_ptr<D3DPipelineObject> GraphicsPipelineIndirectDraw::_createGraphicsPipelineObject(
	ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
	ID3D12RootSignature* graphicsRootSignature
) const noexcept {
	return CreateGraphicsPipelineObjectVS(
		device, shaderPath, pixelShader, L"VertexShaderIndirect.cso", graphicsRootSignature
	);
}

// Individual Draw
GraphicsPipelineIndividualDraw::GraphicsPipelineIndividualDraw() noexcept
	: m_modelCount{ 0u }, m_modelOffset{ 0u } {}

void GraphicsPipelineIndividualDraw::ConfigureGraphicsPipelineObject(
	const std::wstring& pixelShader, size_t modelCount, size_t modelOffset
) noexcept {
	m_pixelShader = pixelShader;
	m_modelCount = modelCount;
	m_modelOffset = modelOffset;
}

void GraphicsPipelineIndividualDraw::DrawModels(
	ID3D12GraphicsCommandList* graphicsCommandList,
	const std::vector<ModelDrawArguments>& drawArguments,
	const RSLayoutType& graphicsRSLayout
) const noexcept {
	for (size_t index = 0u; index < m_modelCount; ++index) {
		const auto& modelArgs = drawArguments[m_modelOffset + index];

		static constexpr size_t modelInfoIndex = static_cast<size_t>(RootSigElement::ModelInfo);

		graphicsCommandList->SetGraphicsRoot32BitConstant(
			graphicsRSLayout[modelInfoIndex], modelArgs.modelIndex, 0u
		);

		const D3D12_DRAW_INDEXED_ARGUMENTS& args = modelArgs.drawIndexed;

		graphicsCommandList->DrawIndexedInstanced(
			args.IndexCountPerInstance, args.InstanceCount, args.StartIndexLocation,
			args.BaseVertexLocation, args.StartInstanceLocation
		);
	}
}

std::unique_ptr<D3DPipelineObject> GraphicsPipelineIndividualDraw::_createGraphicsPipelineObject(
	ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
	ID3D12RootSignature* graphicsRootSignature
) const noexcept {
	return CreateGraphicsPipelineObjectVS(
		device, shaderPath, pixelShader, L"VertexShaderIndividual.cso", graphicsRootSignature
	);
}
