#include <GraphicsPipelineIndirectDraw.hpp>
#include <VertexLayout.hpp>
#include <Shader.hpp>
#include <ComputePipelineIndirectDraw.hpp>

GraphicsPipelineIndirectDraw::GraphicsPipelineIndirectDraw() noexcept
	: m_modelCount{ 0u }, m_counterBufferOffset{ 0u }, m_argumentBufferOffset{ 0u } {}

void GraphicsPipelineIndirectDraw::BindGraphicsPipeline(
	ID3D12GraphicsCommandList* graphicsCommandList, ID3D12RootSignature* graphicsRS
) const noexcept {
	graphicsCommandList->SetPipelineState(m_graphicPSO->Get());
	graphicsCommandList->SetGraphicsRootSignature(graphicsRS);
	graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void GraphicsPipelineIndirectDraw::DrawModels(
	ID3D12CommandSignature* commandSignature, ID3D12GraphicsCommandList* graphicsCommandList,
	ID3D12Resource* argumentBuffer, ID3D12Resource* counterBuffer
) const noexcept {
	graphicsCommandList->ExecuteIndirect(
		commandSignature, m_modelCount, argumentBuffer, m_argumentBufferOffset,
		counterBuffer, m_counterBufferOffset
	);
}

std::unique_ptr<D3DPipelineObject> GraphicsPipelineIndirectDraw::_createGraphicsPipelineObject(
	ID3D12Device* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
	ID3D12RootSignature* graphicsRootSignature
) const noexcept {
	auto vs = std::make_unique<Shader>();
	vs->LoadBinary(shaderPath + L"VertexShader.cso");

	auto ps = std::make_unique<Shader>();
	ps->LoadBinary(shaderPath + pixelShader);

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

void GraphicsPipelineIndirectDraw::ConfigureGraphicsPipelineObject(
	const std::wstring& pixelShader, UINT modelCount, std::uint32_t modelCountOffset,
	size_t counterIndex
) noexcept {
	m_modelCount = modelCount;

	m_argumentBufferOffset = sizeof(IndirectArguments) * modelCountOffset;
	m_counterBufferOffset = sizeof(std::uint32_t) * 2u * counterIndex;
	m_pixelShader = pixelShader;
}

void GraphicsPipelineIndirectDraw::CreateGraphicsPipeline(
	ID3D12Device* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath
) noexcept {
	m_graphicPSO = _createGraphicsPipelineObject(
		device, shaderPath, m_pixelShader, graphicsRootSignature
	);
}
