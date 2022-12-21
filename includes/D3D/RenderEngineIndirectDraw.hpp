#ifndef RENDER_ENGINE_INDIRECT_DRAW_
#define RENDER_ENGINE_INDIRECT_DRAW_
#include <RenderEngine.hpp>
#include <RenderPipelineIndirectDraw.hpp>
#include <D3DPipelineObject.hpp>
#include <RootSignatureDynamic.hpp>

class RenderEngineIndirectDraw final : public RenderEngine {
public:
	void InitiatePipelines(std::uint32_t bufferCount) noexcept override;
	void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) override;
	void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) override;
	void Present(ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex) override;
	void ExecutePostRenderStage() override;
	void ConstructPipelines() override;

	void RecordModelData(
		const std::vector<std::shared_ptr<IModel>>& models
	) noexcept override;
	void CreateBuffers(ID3D12Device* device) override;
	void ReserveBuffers(ID3D12Device* device) override;
	void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept override;
	void ReleaseUploadResources() noexcept override;

private:
	[[nodiscard]]
	std::unique_ptr<RootSignatureDynamic> CreateGraphicsRootSignature(
		ID3D12Device* device
	) const noexcept;
	[[nodiscard]]
	std::unique_ptr<RootSignatureDynamic> CreateComputeRootSignature(
		ID3D12Device* device
	) const noexcept;
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> CreateComputePipelineObject(
		ID3D12Device* device, ID3D12RootSignature* computeRootSignature
	) const noexcept;

	void BindComputePipeline(
		ID3D12GraphicsCommandList* computeCommandList
	) const noexcept;
	void CreateCommandSignature(ID3D12Device* device);

private:
	ComPtr<ID3D12CommandSignature> m_commandSignature;
	std::unique_ptr<RootSignatureBase> m_computeRS;
	std::unique_ptr<D3DPipelineObject> m_computePSO;
	std::unique_ptr<RootSignatureBase> m_graphicsRS;

	std::unique_ptr<RenderPipelineIndirectDraw> m_renderPipeline;
	RSLayoutType m_graphicsRSLayout;
	RSLayoutType m_computeRSLayout;
};
#endif
