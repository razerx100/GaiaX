#ifndef RENDER_ENGINE_BASE_HPP_
#define RENDER_ENGINE_BASE_HPP_
#include <concepts>
#include <RenderEngine.hpp>
#include <GraphicsPipelineBase.hpp>
#include <ViewportAndScissorManager.hpp>
#include <DepthBuffer.hpp>

/*
class RenderEngineBase : public RenderEngine
{
public:
	RenderEngineBase(ID3D12Device* device);

	void Present(size_t frameIndex) final;
	void ExecutePostRenderStage() final;

	void ResizeViewportAndScissor(std::uint32_t width, std::uint32_t height) noexcept final;
	void CreateDepthBufferView(
		ID3D12Device* device, std::uint32_t width, std::uint32_t height
	) final;
	void ReserveBuffers(ID3D12Device* device) final;

	virtual void AddGVerticesAndIndices(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
	) noexcept override;
	virtual void RecordModelDataSet(
		const std::vector<std::shared_ptr<Model>>& models, const std::wstring& pixelShader
	) noexcept override;
	virtual void AddMeshletModelSet(
		std::vector<MeshletModel>& meshletModels, const std::wstring& pixelShader
	) noexcept override;
	virtual void AddGVerticesAndPrimIndices(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
		std::vector<std::uint32_t>&& gPrimIndices
	) noexcept override;

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

	virtual void ReserveBuffersDerived(ID3D12Device* device);

	void ConstructGraphicsRootSignature(ID3D12Device* device);

	template<std::derived_from<GraphicsPipelineBase> Pipeline>
	void CreateGraphicsPipelines(
		ID3D12Device2* device, std::unique_ptr<Pipeline>& graphicsPipeline0,
		std::vector<std::unique_ptr<Pipeline>>& graphicsPipelines
	) const noexcept {
		ID3D12RootSignature* graphicsRootSig = m_graphicsRS->Get();

		graphicsPipeline0->CreateGraphicsPipeline(device, graphicsRootSig, m_shaderPath);
		for (auto& graphicsPipeline : graphicsPipelines)
			graphicsPipeline->CreateGraphicsPipeline(device, graphicsRootSig, m_shaderPath);
	}

protected:
	//std::unique_ptr<RootSignatureBase> m_graphicsRS;
	//RSLayoutType m_graphicsRSLayout;

private:
	ViewportAndScissorManager m_viewportAndScissor;
	//DepthBuffer m_depthBuffer;
};
*/
#endif
