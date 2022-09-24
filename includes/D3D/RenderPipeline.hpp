#ifndef RENDER_PIPELINE_HPP_
#define RENDER_PIPELINE_HPP_

#include <D3DHeaders.hpp>
#include <vector>
#include <memory>
#include <PipelineObjectGFX.hpp>
#include <RootSignatureBase.hpp>
#include <IModel.hpp>
#include <D3DResource.hpp>
#include <D3DDescriptorView.hpp>

struct IndirectCommand {
	std::uint32_t modelIndex;
	D3D12_DRAW_INDEXED_ARGUMENTS drawIndexed;
};

// Need to pack stuff in multiple of 16bytes and since the buffers will be allocated
// contiguously.
struct ModelConstantBuffer {
	UVInfo uvInfo;
	DirectX::XMMATRIX modelMatrix;
	std::uint32_t textureIndex;
	float padding[3];
};

class RenderPipeline {
public:
	RenderPipeline(std::uint32_t frameCount) noexcept;

	void AddGraphicsPipelineObject(std::unique_ptr<PipelineObjectGFX> pso) noexcept;
	void AddGraphicsRootSignature(std::unique_ptr<RootSignatureBase> signature) noexcept;
	void AddComputePipelineObject() noexcept;
	void AddComputeRootSignature(std::unique_ptr<RootSignatureBase> signature) noexcept;

	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void UpdateModelData(size_t frameIndex) const noexcept;
	void BindGraphicsPipeline(ID3D12GraphicsCommandList* graphicsCommandList) const noexcept;
	void DrawModels(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) const noexcept;

	void BindComputePipeline(ID3D12GraphicsCommandList* computeCommandList) const noexcept;
	void DispatchCompute(ID3D12GraphicsCommandList* computeCommandList) const noexcept;

	void CreateCommandSignature(ID3D12Device* device);
	void CreateBuffers(ID3D12Device* device);
	void ReserveBuffers(ID3D12Device* device);

	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

private:
	std::vector<std::shared_ptr<IModel>> m_opaqueModels;

	std::unique_ptr<PipelineObjectGFX> m_graphicPSO;
	std::unique_ptr<RootSignatureBase> m_graphicsRS;

	std::unique_ptr<RootSignatureBase> m_computeRS;

	ComPtr<ID3D12CommandSignature> m_commandSignature;
	UINT m_modelCount;
	std::uint32_t m_frameCount;
	size_t m_modelBufferPerFrameSize;
	D3DUploadResourceDescriptorView m_commandBuffer;
	size_t m_commandDescriptorOffset;
	D3DSingleDescriptorView m_modelBuffers;
};
#endif
