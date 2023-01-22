#ifndef GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#define GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#include <vector>
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineVertexShader : public GraphicsPipelineBase {
private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipelineObject(
		ID3D12Device* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
		ID3D12RootSignature* graphicsRootSignature
	) const noexcept override;
};

class GraphicsPipelineIndirectDraw : public GraphicsPipelineVertexShader {
public:
	GraphicsPipelineIndirectDraw() noexcept;

	void ConfigureGraphicsPipelineObject(
		const std::wstring& pixelShader, UINT modelCount,
		std::uint32_t modelCountOffset, size_t counterIndex
	) noexcept;

	void DrawModels(
		ID3D12CommandSignature* commandSignature, ID3D12GraphicsCommandList* graphicsCommandList,
		ID3D12Resource* argumentBuffer, ID3D12Resource* counterBuffer
	) const noexcept;

private:
	UINT m_modelCount;
	UINT64 m_counterBufferOffset;
	UINT64 m_argumentBufferOffset;
};

class GraphicsPipelineIndividualDraw : public GraphicsPipelineVertexShader {
public:
	GraphicsPipelineIndividualDraw() noexcept;

	void ConfigureGraphicsPipelineObject(
		const std::wstring& pixelShader, size_t modelCount, size_t modelOffset
	) noexcept;

	void DrawModels(
		ID3D12GraphicsCommandList* graphicsCommandList,
		const std::vector<D3D12_DRAW_INDEXED_ARGUMENTS>& drawArguments
	) const noexcept;

private:
	size_t m_modelCount;
	size_t m_modelOffset;
};
#endif
