#include <GraphicsPipelineBase.hpp>

void GraphicsPipelineBase::Bind(const D3DCommandList& graphicsCmdList) const noexcept
{
	ID3D12GraphicsCommandList* cmdList = graphicsCmdList.Get();

	cmdList->SetPipelineState(m_graphicsPipeline->Get());
}

void GraphicsPipelineBase::SetIATopology(const D3DCommandList& graphicsCmdList) noexcept
{
	ID3D12GraphicsCommandList* cmdList = graphicsCmdList.Get();

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
