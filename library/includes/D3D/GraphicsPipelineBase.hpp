#ifndef GRAPHICS_PIPELINE_BASE_HPP_
#define GRAPHICS_PIPELINE_BASE_HPP_
#include <string>
#include <memory>
#include <D3DHeaders.hpp>
#include <D3DPipelineObject.hpp>
#include <D3DRootSignature.hpp>
#include <D3DCommandQueue.hpp>
#include <ExternalPipeline.hpp>
#include <D3DExternalFormatMap.hpp>

template<typename Derived>
class GraphicsPipelineBase
{
public:
	GraphicsPipelineBase() : m_graphicsPipeline{}, m_graphicsExternalPipeline{} {}

	void Create(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
	) {
		m_graphicsExternalPipeline = graphicsExtPipeline;

		m_graphicsPipeline = static_cast<Derived*>(this)->_createGraphicsPipeline(
			device, graphicsRootSignature, shaderPath, m_graphicsExternalPipeline
		);
	}

	void Create(
		ID3D12Device2* device, const D3DRootSignature& graphicsRootSignature,
		const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
	) {
		Create(device, graphicsRootSignature.Get(), shaderPath, graphicsExtPipeline);
	}

	void Recreate(
		ID3D12Device2* device, ID3D12RootSignature* graphicsRootSignature, const std::wstring& shaderPath
	) {
		m_graphicsPipeline = static_cast<Derived*>(this)->_createGraphicsPipeline(
			device, graphicsRootSignature, shaderPath, m_graphicsExternalPipeline
		);
	}

	void Recreate(
		ID3D12Device2* device, const D3DRootSignature& graphicsRootSignature,
		const std::wstring& shaderPath
	) {
		Recreate(device, graphicsRootSignature.Get(), shaderPath);
	}

	void Bind(const D3DCommandList& graphicsCmdList) const noexcept
	{
		ID3D12GraphicsCommandList* cmdList = graphicsCmdList.Get();

		cmdList->SetPipelineState(m_graphicsPipeline->Get());
	}

	[[nodiscard]]
	const ExternalGraphicsPipeline& GetExternalPipeline() const noexcept
	{
		return m_graphicsExternalPipeline;
	}

protected:
	std::unique_ptr<D3DPipelineObject> m_graphicsPipeline;
	ExternalGraphicsPipeline           m_graphicsExternalPipeline;

	static constexpr ShaderType s_shaderBytecodeType = ShaderType::DXIL;

public:
	GraphicsPipelineBase(const GraphicsPipelineBase&) = delete;
	GraphicsPipelineBase& operator=(const GraphicsPipelineBase&) = delete;

	GraphicsPipelineBase(GraphicsPipelineBase&& other) noexcept
		: m_graphicsPipeline{ std::move(other.m_graphicsPipeline) },
		m_graphicsExternalPipeline{ std::move(other.m_graphicsExternalPipeline) }
	{}
	GraphicsPipelineBase& operator=(GraphicsPipelineBase&& other) noexcept
	{
		m_graphicsPipeline         = std::move(other.m_graphicsPipeline);
		m_graphicsExternalPipeline = std::move(other.m_graphicsExternalPipeline);

		return *this;
	}
};

template<typename Pipeline_t>
void ConfigurePipelineBuilder(
	GraphicsPipelineBuilder<Pipeline_t>& builder, const ExternalGraphicsPipeline& graphicsExtPipeline
) noexcept {
	if (graphicsExtPipeline.GetBackfaceCullingState())
		builder.SetCullMode(D3D12_CULL_MODE_BACK);

	{
		DXGI_FORMAT depthFormat   = GetDxgiFormat(graphicsExtPipeline.GetDepthFormat());
		DXGI_FORMAT stencilFormat = GetDxgiFormat(graphicsExtPipeline.GetStencilFormat());

		const BOOL isDepthFormatValid   = depthFormat != DXGI_FORMAT_UNKNOWN;
		const BOOL isStencilFormatValid = stencilFormat != DXGI_FORMAT_UNKNOWN;

		const D3D12_DEPTH_WRITE_MASK depthWrite
			= graphicsExtPipeline.IsDepthWriteEnabled()
				? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

		DepthStencilStateBuilder depthStencilBuilder{};

		if (isDepthFormatValid || isStencilFormatValid)
			depthStencilBuilder.Enable(isDepthFormatValid, depthWrite, isStencilFormatValid);

		DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_UNKNOWN;

		// There is no Stencil only format in DXGI. So, if there is only a depth buffer, then
		// the format can be different. But if there is only a stencil or both a depth and stencil
		// the format should be the same.
		if (isDepthFormatValid)
			depthStencilFormat = depthFormat;
		else if (isStencilFormatValid)
			depthStencilFormat = stencilFormat;

		builder.SetDepthStencilState(depthStencilBuilder, depthStencilFormat);
	}

	const size_t renderTargetCount = graphicsExtPipeline.GetRenderTargetCount();

	for (size_t index = 0u; index < renderTargetCount; ++index)
		builder.AddRenderTarget(
			GetDxgiFormat(graphicsExtPipeline.GetRenderTargetFormat(index)),
			GetD3DBlendState(graphicsExtPipeline.GetBlendState(index))
		);
}
#endif
