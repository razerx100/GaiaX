#ifndef GRAPHICS_PIPELINE_VS_HPP_
#define GRAPHICS_PIPELINE_VS_HPP_
#include <GraphicsPipelineBase.hpp>

namespace GraphicsPipelineVS
{
	void SetIATopology(const D3DCommandList& graphicsCmdList) noexcept;
}

class GraphicsPipelineVSIndirectDraw : public GraphicsPipelineBase<GraphicsPipelineVSIndirectDraw>
{
	friend class GraphicsPipelineBase<GraphicsPipelineVSIndirectDraw>;

public:
	GraphicsPipelineVSIndirectDraw() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) const;

public:
	GraphicsPipelineVSIndirectDraw(const GraphicsPipelineVSIndirectDraw&) = delete;
	GraphicsPipelineVSIndirectDraw& operator=(const GraphicsPipelineVSIndirectDraw&) = delete;

	GraphicsPipelineVSIndirectDraw(GraphicsPipelineVSIndirectDraw&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineVSIndirectDraw& operator=(GraphicsPipelineVSIndirectDraw&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};

class GraphicsPipelineVSIndividualDraw : public GraphicsPipelineBase<GraphicsPipelineVSIndividualDraw>
{
	friend class GraphicsPipelineBase<GraphicsPipelineVSIndividualDraw>;

public:
	GraphicsPipelineVSIndividualDraw() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) const;

public:
	GraphicsPipelineVSIndividualDraw(const GraphicsPipelineVSIndividualDraw&) = delete;
	GraphicsPipelineVSIndividualDraw& operator=(const GraphicsPipelineVSIndividualDraw&) = delete;

	GraphicsPipelineVSIndividualDraw(GraphicsPipelineVSIndividualDraw&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineVSIndividualDraw& operator=(GraphicsPipelineVSIndividualDraw&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};
#endif