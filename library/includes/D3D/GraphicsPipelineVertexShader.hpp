#ifndef GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#define GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#include <vector>
#include <GraphicsPipelineBase.hpp>
#include <GaiaDataTypes.hpp>
#include <RootSignatureDynamic.hpp>

class GraphicsPipelineVertexShader : public GraphicsPipelineBase {
protected:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineObjectVS(
		ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
		const std::wstring& vertexShader, ID3D12RootSignature* graphicsRootSignature
	) const noexcept;
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
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipelineObject(
		ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
		ID3D12RootSignature* graphicsRootSignature
	) const noexcept override;

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
		const std::vector<ModelDrawArguments>& drawArguments,
		const RSLayoutType& graphicsRSLayout
	) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipelineObject(
		ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
		ID3D12RootSignature* graphicsRootSignature
	) const noexcept override;

private:
	size_t m_modelCount;
	size_t m_modelOffset;
};
#endif
