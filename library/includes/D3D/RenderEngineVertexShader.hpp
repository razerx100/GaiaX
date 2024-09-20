#ifndef RENDER_ENGINE_VERTEX_SHADER_HPP_
#define RENDER_ENGINE_VERTEX_SHADER_HPP_
#include <RenderEngineBase.hpp>
#include <ModelManager.hpp>

class RenderEngineVSIndividual
	: public RenderEngineCommon<ModelManagerVSIndividual, RenderEngineVSIndividual>
{
	friend class RenderEngineCommon<ModelManagerVSIndividual, RenderEngineVSIndividual>;

public:
	RenderEngineVSIndividual(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& pixelShader
	) override;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;

private:
	[[nodiscard]]
	static ModelManagerVSIndividual GetModelManager(
		const DeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

	[[nodiscard]]
	ID3D12Fence* GenericCopyStage(
		size_t frameIndex, ID3D12Resource* frameBuffer, UINT64& counterValue, ID3D12Fence* waitFence
	);
	[[nodiscard]]
	ID3D12Fence* DrawingStage(
		size_t frameIndex, ID3D12Resource* frameBuffer, UINT64& counterValue, ID3D12Fence* waitFence
	);

	void SetupPipelineStages();

private:
	static constexpr size_t s_cameraRegisterSlot = 1u;

public:
	RenderEngineVSIndividual(const RenderEngineVSIndividual&) = delete;
	RenderEngineVSIndividual& operator=(const RenderEngineVSIndividual&) = delete;

	RenderEngineVSIndividual(RenderEngineVSIndividual&& other) noexcept
		: RenderEngineCommon{ std::move(other) }
	{}
	RenderEngineVSIndividual& operator=(RenderEngineVSIndividual&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));

		return *this;
	}
};

/*
class RenderEngineVertexShader : public RenderEngineBase {
public:
	RenderEngineVertexShader(ID3D12Device* device);

	void AddGVerticesAndIndices(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
	) noexcept final;
	void ExecuteRenderStage(size_t frameIndex) final;

	void CreateBuffers(ID3D12Device* device) final;
	void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept final;
	void ReleaseUploadResources() noexcept final;

protected:
	[[nodiscard]]
	std::unique_ptr<RootSignatureDynamic> CreateGraphicsRootSignature(
		ID3D12Device* device
	) const noexcept final;

	void ReserveBuffersDerived(ID3D12Device* device) final;

	void BindGraphicsBuffers(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	);

	virtual void _createBuffers(ID3D12Device* device);
	virtual void _reserveBuffers(ID3D12Device* device);
	virtual void _recordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept;
	virtual void _releaseUploadResources() noexcept;

	virtual void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) = 0;
	virtual void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) = 0;

private:
	//VertexManagerVertexShader m_vertexManager;
};

class RenderEngineIndirectDraw final : public RenderEngineVertexShader {
public:
	struct Args {
		std::optional<ID3D12Device*> device;
		std::optional<std::uint32_t> frameCount;
	};

public:
	RenderEngineIndirectDraw(ID3D12Device* device, std::uint32_t frameCount);

	void ConstructPipelines() final;

	void RecordModelDataSet(
		const std::vector<std::shared_ptr<Model>>& models, const std::wstring& pixelShader
	) noexcept final;

private:
	void _createBuffers(ID3D12Device* device) override;
	void _reserveBuffers(ID3D12Device* device) override;
	void _recordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept override;
	void _releaseUploadResources() noexcept override;

	void CreateCommandSignature(ID3D12Device* device);
	void ExecuteComputeStage(size_t frameIndex);

	void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) final;
	void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) final;
	void UpdateModelBuffers(size_t frameIndex) const noexcept final;

	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndirectDraw>;

private:
	//ComputePipelineIndirectDraw m_computePipeline;
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;

	ComPtr<ID3D12CommandSignature> m_commandSignature;
};

class RenderEngineIndividualDraw final : public RenderEngineVertexShader {
public:
	RenderEngineIndividualDraw(ID3D12Device* device);

	void ConstructPipelines() final;

	void RecordModelDataSet(
		const std::vector<std::shared_ptr<Model>>& models, const std::wstring& pixelShader
	) noexcept final;
	void UpdateModelBuffers(size_t frameIndex) const noexcept final;

private:
	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndividualDraw>;

	void RecordModelArguments(const std::vector<std::shared_ptr<Model>>& models) noexcept;

	void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) final;
	void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) final;

private:
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;

	//std::vector<ModelDrawArguments> m_modelArguments;
};
*/
#endif
