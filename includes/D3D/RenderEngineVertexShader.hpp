#ifndef RENDER_ENGINE_VERTEX_SHADER_HPP_
#define RENDER_ENGINE_VERTEX_SHADER_HPP_
#include <RenderEngineBase.hpp>
#include <ComputePipelineIndirectDraw.hpp>
#include <GraphicsPipelineVertexShader.hpp>
#include <VertexManagerVertexShader.hpp>
#include <optional>

class RenderEngineVertexShader : public RenderEngineBase {
public:
	void AddGlobalVertices(
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	) noexcept override;

	void CreateBuffers(ID3D12Device* device) override;
	void ReserveBuffers(ID3D12Device* device) override;
	void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept override;
	void ReleaseUploadResources() noexcept override;
	void CopyData(std::atomic_size_t& workCount) const noexcept override;

protected:
	[[nodiscard]]
	std::unique_ptr<RootSignatureDynamic> CreateGraphicsRootSignature(
		ID3D12Device* device
	) const noexcept override;

	void BindGraphicsBuffers(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	);

	virtual void _createBuffers(ID3D12Device* device);
	virtual void _reserveBuffers(ID3D12Device* device);
	virtual void _recordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept;
	virtual void _releaseUploadResources() noexcept;

private:
	VertexManagerVertexShader m_vertexManager;
};

class RenderEngineIndirectDraw final : public RenderEngineVertexShader {
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
	void ConstructPipelines() override;

	void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& pixelShader
	) noexcept override;

private:
	void _createBuffers(ID3D12Device* device) override;
	void _reserveBuffers(ID3D12Device* device) override;
	void _recordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept override;
	void _releaseUploadResources() noexcept override;

	void CreateCommandSignature(ID3D12Device* device);
	void ExecuteComputeStage(size_t frameIndex);

	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndirectDraw>;

private:
	ComputePipelineIndirectDraw m_computePipeline;
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;

	ComPtr<ID3D12CommandSignature> m_commandSignature;
};

class RenderEngineIndividualDraw final : public RenderEngineVertexShader {
public:
	void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) override;
	void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) override;
	void ConstructPipelines() override;

	void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& pixelShader
	) noexcept override;

private:
	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndividualDraw>;

	void RecordModelArguments(const std::vector<std::shared_ptr<IModel>>& models) noexcept;

private:
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;

	std::vector<ModelDrawArguments> m_modelArguments;
};
#endif
