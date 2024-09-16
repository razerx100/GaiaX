#include <GraphicsPipelineBase.hpp>

void GraphicsPipelineBase::Bind(const D3DCommandList& graphicsCmdList) const noexcept
{
	ID3D12GraphicsCommandList* cmdList = graphicsCmdList.Get();

	cmdList->SetPipelineState(m_graphicsPipeline->Get());
}
