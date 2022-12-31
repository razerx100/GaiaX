#ifndef RENDER_ENGINE_INDIRECT_DRAW_
#define RENDER_ENGINE_INDIRECT_DRAW_
#include <RenderEngine.hpp>
#include <ComputePipelineIndirectDraw.hpp>
#include <GraphicsPipelineIndirectDraw.hpp>
#include <RootSignatureDynamic.hpp>
#include <VertexManagerVertex.hpp>
#include <optional>

class RenderEngineIndirectDraw final : public RenderEngine {
public:
	struct Args {
		std::optional<std::uint32_t> frameCount;
	};

public:
	RenderEngineIndirectDraw(const Args& arguments);

	void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) override;
	void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) override;
	void Present(ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex) override;
	void ExecutePostRenderStage() override;
	void ConstructPipelines() override;

	void AddGlobalVertices(
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	) noexcept override;
	void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& pixelShader
	) noexcept override;
	void CreateBuffers(ID3D12Device* device) override;
	void ReserveBuffers(ID3D12Device* device) override;
	void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept override;
	void ReleaseUploadResources() noexcept override;
	void CopyData(std::atomic_size_t& workCount) const noexcept override;

private:
	[[nodiscard]]
	std::unique_ptr<RootSignatureDynamic> CreateGraphicsRootSignature(
		ID3D12Device* device
	) const noexcept;

	void CreateCommandSignature(ID3D12Device* device);

	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndirectDraw>;

private:
	ComputePipelineIndirectDraw m_computePipeline;
	VertexManagerVertex m_vertexManager;
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;

	std::unique_ptr<RootSignatureBase> m_graphicsRS;
	ComPtr<ID3D12CommandSignature> m_commandSignature;
	RSLayoutType m_graphicsRSLayout;
};
#endif
