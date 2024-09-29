#ifndef GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#define GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#include <GraphicsPipelineBase.hpp>

namespace GraphicsPipelineVertexShader
{
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineVS(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		ShaderType binaryType, const std::wstring& shaderPath, const ShaderName& pixelShader,
		const ShaderName& vertexShader
	);
	void SetIATopology(const D3DCommandList& graphicsCmdList) noexcept;
}

class GraphicsPipelineIndirectDraw : public GraphicsPipelineBase<GraphicsPipelineIndirectDraw>
{
	friend class GraphicsPipelineBase<GraphicsPipelineIndirectDraw>;

public:
	GraphicsPipelineIndirectDraw() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) const;

public:
	GraphicsPipelineIndirectDraw(const GraphicsPipelineIndirectDraw&) = delete;
	GraphicsPipelineIndirectDraw& operator=(const GraphicsPipelineIndirectDraw&) = delete;

	GraphicsPipelineIndirectDraw(GraphicsPipelineIndirectDraw&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineIndirectDraw& operator=(GraphicsPipelineIndirectDraw&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};

class GraphicsPipelineIndividualDraw : public GraphicsPipelineBase<GraphicsPipelineIndividualDraw>
{
	friend class GraphicsPipelineBase<GraphicsPipelineIndividualDraw>;

public:
	GraphicsPipelineIndividualDraw() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) const;

public:
	GraphicsPipelineIndividualDraw(const GraphicsPipelineIndividualDraw&) = delete;
	GraphicsPipelineIndividualDraw& operator=(const GraphicsPipelineIndividualDraw&) = delete;

	GraphicsPipelineIndividualDraw(GraphicsPipelineIndividualDraw&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineIndividualDraw& operator=(GraphicsPipelineIndividualDraw&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};
#endif
