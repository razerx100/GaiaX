#ifndef GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#define GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineVertexShader : public GraphicsPipelineBase
{
public:
	GraphicsPipelineVertexShader() : GraphicsPipelineBase{} {}

	void Create(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) final;

	using GraphicsPipelineBase::Create;

protected:
	[[nodiscard]]
	static std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineVS(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader,
		const ShaderName& vertexShader
	);
	[[nodiscard]]
	virtual std::unique_ptr<D3DPipelineObject> _createGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) const = 0;

public:
	GraphicsPipelineVertexShader(const GraphicsPipelineVertexShader&) = delete;
	GraphicsPipelineVertexShader& operator=(const GraphicsPipelineVertexShader&) = delete;

	GraphicsPipelineVertexShader(GraphicsPipelineVertexShader&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineVertexShader& operator=(GraphicsPipelineVertexShader&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};

class GraphicsPipelineIndirectDraw : public GraphicsPipelineVertexShader
{
public:
	GraphicsPipelineIndirectDraw() : GraphicsPipelineVertexShader{} {}

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) const final;

public:
	GraphicsPipelineIndirectDraw(const GraphicsPipelineIndirectDraw&) = delete;
	GraphicsPipelineIndirectDraw& operator=(const GraphicsPipelineIndirectDraw&) = delete;

	GraphicsPipelineIndirectDraw(GraphicsPipelineIndirectDraw&& other) noexcept
		: GraphicsPipelineVertexShader{ std::move(other) }
	{}
	GraphicsPipelineIndirectDraw& operator=(GraphicsPipelineIndirectDraw&& other) noexcept
	{
		GraphicsPipelineVertexShader::operator=(std::move(other));

		return *this;
	}
};

class GraphicsPipelineIndividualDraw : public GraphicsPipelineVertexShader
{
public:
	GraphicsPipelineIndividualDraw() : GraphicsPipelineVertexShader{} {}

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipeline(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) const final;

public:
	GraphicsPipelineIndividualDraw(const GraphicsPipelineIndividualDraw&) = delete;
	GraphicsPipelineIndividualDraw& operator=(const GraphicsPipelineIndividualDraw&) = delete;

	GraphicsPipelineIndividualDraw(GraphicsPipelineIndividualDraw&& other) noexcept
		: GraphicsPipelineVertexShader{ std::move(other) }
	{}
	GraphicsPipelineIndividualDraw& operator=(GraphicsPipelineIndividualDraw&& other) noexcept
	{
		GraphicsPipelineVertexShader::operator=(std::move(other));

		return *this;
	}
};
#endif
