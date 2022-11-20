#ifndef RENDER_PIPELINE_HPP_
#define RENDER_PIPELINE_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <memory>
#include <D3DPipelineObject.hpp>
#include <RootSignatureBase.hpp>
#include <IModel.hpp>
#include <D3DResource.hpp>
#include <D3DDescriptorView.hpp>

struct IndirectCommand {
	std::uint32_t modelIndex;
	D3D12_DRAW_INDEXED_ARGUMENTS drawIndexed;
};

struct CullingData {
	std::uint32_t commandCount;
};

class RenderPipeline {
public:
	RenderPipeline(std::uint32_t frameCount) noexcept;

	void AddGraphicsPipelineObject(std::unique_ptr<D3DPipelineObject> pso) noexcept;
	void AddGraphicsRootSignature(std::unique_ptr<RootSignatureBase> signature) noexcept;
	void AddComputePipelineObject(std::unique_ptr<D3DPipelineObject> pso) noexcept;
	void AddComputeRootSignature(std::unique_ptr<RootSignatureBase> signature) noexcept;

	void RecordIndirectArguments(const std::vector<std::shared_ptr<IModel>>& models) noexcept;

	void BindGraphicsPipeline(ID3D12GraphicsCommandList* graphicsCommandList) const noexcept;
	void DrawModels(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) const noexcept;

	void BindComputePipeline(ID3D12GraphicsCommandList* computeCommandList) const noexcept;
	void DispatchCompute(
		ID3D12GraphicsCommandList* computeCommandList, size_t frameIndex
	) const noexcept;
	void ResetCounterBuffer(
		ID3D12GraphicsCommandList* commandList, size_t frameIndex
	) const noexcept;

	void CreateCommandSignature(ID3D12Device* device);
	void CreateBuffers(ID3D12Device* device);
	void ReserveBuffers(ID3D12Device* device);

	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

	[[nodiscard]]
	D3D12_RESOURCE_BARRIER GetTransitionBarrier(
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState, size_t frameIndex
	) const noexcept;

private:
	std::unique_ptr<D3DPipelineObject> m_graphicPSO;
	std::unique_ptr<RootSignatureBase> m_graphicsRS;

	std::unique_ptr<D3DPipelineObject> m_computePSO;
	std::unique_ptr<RootSignatureBase> m_computeRS;

	ComPtr<ID3D12CommandSignature> m_commandSignature;
	UINT m_modelCount;
	std::uint32_t m_frameCount;
	size_t m_modelBufferPerFrameSize;
	D3DUploadResourceDescriptorView m_commandBufferSRV;
	std::vector<D3DSingleDescriptorView> m_commandBufferUAVs;
	D3DResourceView m_uavCounterBuffer;
	D3DUploadableResourceView m_cullingDataBuffer;
	size_t m_commandDescriptorOffset;
	std::vector<IndirectCommand> m_indirectCommands;

	static constexpr float THREADBLOCKSIZE = 128.f;
};
#endif
