#include <GraphicsPipelineBase.hpp>

void GraphicsPipelineBase::BindGraphicsPipeline(
	ID3D12GraphicsCommandList* graphicsCommandList, ID3D12RootSignature* graphicsRS
) const noexcept {
	graphicsCommandList->SetPipelineState(m_graphicPSO->Get());
	graphicsCommandList->SetGraphicsRootSignature(graphicsRS);
	graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void GraphicsPipelineBase::CreateGraphicsPipeline(
	ID3D12Device* device, ID3D12RootSignature* graphicsRootSignature,
	const std::wstring& shaderPath
) noexcept {
	m_graphicPSO = _createGraphicsPipelineObject(
		device, shaderPath, m_pixelShader, graphicsRootSignature
	);
}
