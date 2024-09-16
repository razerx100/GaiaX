#ifndef GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#define GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineMeshShader : public GraphicsPipelineBase
{
public:
	GraphicsPipelineMeshShader(bool useAmplificationShader = true)
		: GraphicsPipelineBase{}, m_useAmplificationShader{ useAmplificationShader }
	{}

	void Create(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) final;

	using GraphicsPipelineBase::Create;

private:
	[[nodiscard]]
	static std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineMS(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader,
		const ShaderName& meshShader, const ShaderName& amplificationShader = {}
	);

private:
	bool m_useAmplificationShader;

public:
	GraphicsPipelineMeshShader(const GraphicsPipelineMeshShader&) = delete;
	GraphicsPipelineMeshShader& operator=(const GraphicsPipelineMeshShader&) = delete;

	GraphicsPipelineMeshShader(GraphicsPipelineMeshShader&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) },
		m_useAmplificationShader{ other.m_useAmplificationShader }
	{}
	GraphicsPipelineMeshShader& operator=(GraphicsPipelineMeshShader&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));
		m_useAmplificationShader = other.m_useAmplificationShader;

		return *this;
	}
};
#endif
