#ifndef GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#define GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineMeshShader : public GraphicsPipelineBase<GraphicsPipelineMeshShader>
{
	friend class GraphicsPipelineBase<GraphicsPipelineMeshShader>;

public:
	GraphicsPipelineMeshShader() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	static std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineMS(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		ShaderType binaryType, const std::wstring& shaderPath, const ShaderName& pixelShader,
		const ShaderName& meshShader, const ShaderName& amplificationShader
	);

	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) const;

public:
	GraphicsPipelineMeshShader(const GraphicsPipelineMeshShader&) = delete;
	GraphicsPipelineMeshShader& operator=(const GraphicsPipelineMeshShader&) = delete;

	GraphicsPipelineMeshShader(GraphicsPipelineMeshShader&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineMeshShader& operator=(GraphicsPipelineMeshShader&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};
#endif
