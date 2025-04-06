#ifndef GRAPHICS_PIPELINE_MS_HPP_
#define GRAPHICS_PIPELINE_MS_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineMS : public GraphicsPipelineBase<GraphicsPipelineMS>
{
	friend class GraphicsPipelineBase<GraphicsPipelineMS>;

public:
	GraphicsPipelineMS() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	static std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineMS(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		ShaderBinaryType binaryType, const std::wstring& shaderPath,
		const ExternalGraphicsPipeline& graphicsExtPipeline, const ShaderName& amplificationShader
	);

	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
	) const;

public:
	GraphicsPipelineMS(const GraphicsPipelineMS&) = delete;
	GraphicsPipelineMS& operator=(const GraphicsPipelineMS&) = delete;

	GraphicsPipelineMS(GraphicsPipelineMS&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineMS& operator=(GraphicsPipelineMS&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};
#endif
