#ifndef RENDER_PIPELINE_INDIRECT_DRAW_HPP_
#define RENDER_PIPELINE_INDIRECT_DRAW_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <memory>
#include <D3DPipelineObject.hpp>
#include <RootSignatureBase.hpp>
#include <IModel.hpp>
#include <D3DResource.hpp>
#include <D3DDescriptorView.hpp>
#include <RootSignatureDynamic.hpp>

struct IndirectCommand {
	std::uint32_t modelIndex;
	D3D12_DRAW_INDEXED_ARGUMENTS drawIndexed;
};

struct CullingData {
	std::uint32_t commandCount;
	DirectX::XMFLOAT2 xBounds;
	float padding;
	DirectX::XMFLOAT2 yBounds;
	DirectX::XMFLOAT2 zBounds;
};

class RenderPipelineIndirectDraw {
public:
	RenderPipelineIndirectDraw(std::uint32_t frameCount) noexcept;

	void ConfigureGraphicsPipelineObject(
		ID3D12Device* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
		ID3D12RootSignature* graphicsRootSignature
	) noexcept;

	void RecordIndirectArguments(const std::vector<std::shared_ptr<IModel>>& models) noexcept;

	void BindGraphicsPipeline(
		ID3D12GraphicsCommandList* graphicsCommandList, ID3D12RootSignature* graphicsRS
	) const noexcept;
	void DrawModels(
		ID3D12CommandSignature* commandSignature,
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) const noexcept;

	void DispatchCompute(
		ID3D12GraphicsCommandList* computeCommandList, size_t frameIndex,
		const RSLayoutType& computeLayout
	) const noexcept;
	void ResetCounterBuffer(
		ID3D12GraphicsCommandList* commandList, size_t frameIndex
	) const noexcept;

	void CreateBuffers(ID3D12Device* device);
	void ReserveBuffers(ID3D12Device* device);

	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

	[[nodiscard]]
	ID3D12Resource* GetArgumentBuffer(size_t frameIndex) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> CreateGraphicsPipelineObject(
		ID3D12Device* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
		ID3D12RootSignature* graphicsRootSignature
	) const noexcept;

private:
	std::unique_ptr<D3DPipelineObject> m_graphicPSO;

	UINT m_modelCount;
	std::uint32_t m_frameCount;
	D3DUploadResourceDescriptorView m_commandBufferSRV;
	std::vector<D3DSingleDescriptorView> m_commandBufferUAVs;
	D3DResourceView m_uavCounterBuffer;
	D3DUploadableResourceView m_cullingDataBuffer;
	std::vector<IndirectCommand> m_indirectCommands;

	static constexpr float THREADBLOCKSIZE = 64.f;
	static constexpr DirectX::XMFLOAT2 XBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 YBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 ZBOUNDS = { 1.f, -1.f };
};
#endif
