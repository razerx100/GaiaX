#ifndef GRAPHICS_PIPELINE_INDIRECT_DRAW_HPP_
#define GRAPHICS_PIPELINE_INDIRECT_DRAW_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <string>
#include <D3DPipelineObject.hpp>

class GraphicsPipelineIndirectDraw {
public:
	GraphicsPipelineIndirectDraw() noexcept;

	void ConfigureGraphicsPipelineObject(
		const std::wstring& pixelShader, UINT modelCount,
		std::uint32_t modelCountOffset, size_t counterIndex
	) noexcept;
	void CreateGraphicsPipeline(
		ID3D12Device* device, ID3D12RootSignature* graphicsRootSignature,
		const std::wstring& shaderPath
	) noexcept;

	void BindGraphicsPipeline(
		ID3D12GraphicsCommandList* graphicsCommandList, ID3D12RootSignature* graphicsRS
	) const noexcept;
	void DrawModels(
		ID3D12CommandSignature* commandSignature, ID3D12GraphicsCommandList* graphicsCommandList,
		ID3D12Resource* argumentBuffer, ID3D12Resource* counterBuffer
	) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipelineObject(
		ID3D12Device* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
		ID3D12RootSignature* graphicsRootSignature
	) const noexcept;

private:
	std::unique_ptr<D3DPipelineObject> m_graphicPSO;

	UINT m_modelCount;
	UINT64 m_counterBufferOffset;
	UINT64 m_argumentBufferOffset;
	std::wstring m_pixelShader;
};
#endif
