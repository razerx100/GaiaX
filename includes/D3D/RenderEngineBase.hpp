#ifndef RENDER_ENGINE_BASE_HPP_
#define RENDER_ENGINE_BASE_HPP_
#include <concepts>
#include <RenderEngine.hpp>
#include <RootSignatureDynamic.hpp>
#include <GraphicsPipelineBase.hpp>

class RenderEngineBase : public RenderEngine {
public:
	void Present(ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex) override;
	void ExecutePostRenderStage() override;

protected:
	void ExecutePreGraphicsStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	);
	void BindCommonGraphicsBuffers(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	);

	[[nodiscard]]
	virtual std::unique_ptr<RootSignatureDynamic> CreateGraphicsRootSignature(
		ID3D12Device* device
	) const noexcept = 0;

	void ConstructGraphicsRootSignature(ID3D12Device* device);

	template<std::derived_from<GraphicsPipelineBase> Pipeline>
	void CreateGraphicsPipelines(
		ID3D12Device* device, std::unique_ptr<Pipeline>& graphicsPipeline0,
		std::vector<std::unique_ptr<Pipeline>>& graphicsPipelines
	) const noexcept {
		ID3D12RootSignature* graphicsRootSig = m_graphicsRS->Get();

		graphicsPipeline0->CreateGraphicsPipeline(device, graphicsRootSig, m_shaderPath);
		for (auto& graphicsPipeline : graphicsPipelines)
			graphicsPipeline->CreateGraphicsPipeline(device, graphicsRootSig, m_shaderPath);
	}

protected:
	std::unique_ptr<RootSignatureBase> m_graphicsRS;
	RSLayoutType m_graphicsRSLayout;
};
#endif
