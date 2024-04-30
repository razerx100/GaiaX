#ifndef GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#define GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#include <vector>
#include <GraphicsPipelineBase.hpp>
#include <RootSignatureDynamic.hpp>

class GraphicsPipelineMeshShader : public GraphicsPipelineBase {
public:
	void ConfigureGraphicsPipelineObject(const std::wstring& pixelShader) noexcept;
	void AddModelDetails(
		std::uint32_t meshletCount, std::uint32_t meshletOffset, std::uint32_t modelIndex
	) noexcept;
	void DrawModels(
		ID3D12GraphicsCommandList6* graphicsCommandList, const RSLayoutType& graphicsRSLayout
	) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createGraphicsPipelineObject(
		ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
		ID3D12RootSignature* graphicsRootSignature
	) const noexcept override;

private:
	struct ModelDetails {
		std::uint32_t modelIndex;
		std::uint32_t meshletOffset;
		std::uint32_t meshletCount;
	};

private:
	std::vector<ModelDetails> m_modelDetails;
};
#endif
