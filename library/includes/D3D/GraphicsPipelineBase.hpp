#ifndef GRAPHICS_PIPELINE_BASE_HPP_
#define GRAPHICS_PIPELINE_BASE_HPP_
#include <string>
#include <memory>
#include <D3DHeaders.hpp>
#include <D3DPipelineObject.hpp>
#include <D3DRootSignature.hpp>
#include <D3DCommandQueue.hpp>
#include <Shader.hpp>

class GraphicsPipelineBase
{
public:
	GraphicsPipelineBase() : m_graphicsPipeline{}, m_pixelShader{} {}

	virtual void Create(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) = 0;

	void Create(
		ID3D12Device2* device, const D3DRootSignature& graphicsRootSignature,
		const std::wstring& shaderPath, const ShaderName& pixelShader
	) {
		Create(device, graphicsRootSignature.Get(), shaderPath, pixelShader);
	}

	void Bind(const D3DCommandList& graphicsCmdList) const noexcept;

	[[nodiscard]]
	ShaderName GetPixelShader() const noexcept { return m_pixelShader; }

protected:
	std::unique_ptr<D3DPipelineObject> m_graphicsPipeline;
	ShaderName                         m_pixelShader;

	static constexpr ShaderType s_shaderBytecodeType = ShaderType::DXIL;

public:
	GraphicsPipelineBase(const GraphicsPipelineBase&) = delete;
	GraphicsPipelineBase& operator=(const GraphicsPipelineBase&) = delete;

	GraphicsPipelineBase(GraphicsPipelineBase&& other) noexcept
		: m_graphicsPipeline{ std::move(other.m_graphicsPipeline) },
		m_pixelShader{ std::move(other.m_pixelShader) }
	{}
	GraphicsPipelineBase& operator=(GraphicsPipelineBase&& other) noexcept
	{
		m_graphicsPipeline = std::move(other.m_graphicsPipeline);
		m_pixelShader      = std::move(other.m_pixelShader);

		return *this;
	}
};
#endif
