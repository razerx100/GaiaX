#ifndef RENDER_PIPELINE_HPP_
#define RENDER_PIPELINE_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <memory>
#include <PipelineObjectGFX.hpp>
#include <RootSignatureBase.hpp>
#include <D3DBuffer.hpp>
#include <UploadBuffer.hpp>
#include <IModel.hpp>
#include <ConstantBuffer.hpp>
#include <GaiaDataTypes.hpp>

struct IndirectCommand {
	D3D12_GPU_VIRTUAL_ADDRESS cbv;
	D3D12_DRAW_INDEXED_ARGUMENTS drawIndexed;
};

// Need pack stuff in multiple of 16bytes
struct ModelConstantBuffer {
	UVInfo uvInfo;
	DirectX::XMMATRIX modelMatrix;
	std::uint32_t textureIndex;
	double padding[20];
};

class RenderPipeline {
public:
	RenderPipeline(std::uint32_t frameCount) noexcept;

	void AddGraphicsPipelineObject(std::unique_ptr<PipelineObjectGFX> pso) noexcept;
	void AddGraphicsRootSignature(std::unique_ptr<RootSignatureBase> signature) noexcept;
	void AddComputePipelineObject() noexcept;
	void AddComputeRootSignature(std::unique_ptr<RootSignatureBase> signature) noexcept;

	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void UpdateModels(size_t frameIndex) const noexcept;
	void BindGraphicsPipeline(ID3D12GraphicsCommandList* graphicsCommandList) const noexcept;
	void DrawModels(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) const noexcept;

	void BindComputePipeline(ID3D12GraphicsCommandList* computeCommandList) const noexcept;
	void DispatchCompute(ID3D12GraphicsCommandList* computeCommandList) const noexcept;

	void CreateCommandSignature(ID3D12Device* device);
	void CreateCommandBuffers(ID3D12Device* device);
	void ReserveCommandBuffers();

private:
	std::vector<std::shared_ptr<IModel>> m_opaqueModels;

	std::unique_ptr<PipelineObjectGFX> m_graphicPSO;
	std::unique_ptr<RootSignatureBase> m_graphicsRS;

	std::unique_ptr<RootSignatureBase> m_computeRS;

	ComPtr<ID3D12CommandSignature> m_commandSignature;
	UINT m_modelCount;
	std::uint32_t m_frameCount;
	size_t m_modelBufferPerFrameSize;
	D3DBufferShared m_commandBuffer;
	UploadBufferShared m_commandUploadBuffer;
	SharedCPUHandle m_commandDescriptorHandle;
	CPUConstantBuffer m_modelsConstantBuffer;
};
#endif
