#ifndef COMPUTE_PIPELINE_INDIRECT_DRAW_HPP_
#define COMPUTE_PIPELINE_INDIRECT_DRAW_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <string>
#include <vector>
#include <D3DPipelineObject.hpp>
#include <D3DDescriptorView.hpp>
#include <GaiaDataTypes.hpp>
#include <Model.hpp>

class ComputePipelineIndirectDraw {
public:
	ComputePipelineIndirectDraw(std::uint32_t frameCount);

	void RecordIndirectArguments(const std::vector<std::shared_ptr<Model>>& models) noexcept;

	void CreateComputeRootSignature(ID3D12Device* device) noexcept;
	void CreateComputePipelineObject(
		ID3D12Device2* device, const std::wstring& shaderPath
	) noexcept;
	void CreateBuffers(ID3D12Device* device);
	void ReserveBuffers(ID3D12Device* device);
	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

	void BindComputePipeline(
		ID3D12GraphicsCommandList* computeCommandList
	) const noexcept;
	void DispatchCompute(
		ID3D12GraphicsCommandList* computeCommandList, size_t frameIndex
	) const noexcept;
	void ResetCounterBuffer(
		ID3D12GraphicsCommandList* commandList, size_t frameIndex
	) const noexcept;

	[[nodiscard]]
	UINT GetCurrentModelCount() const noexcept;
	[[nodiscard]]
	size_t GetCounterCount() const noexcept;
	//[[nodiscard]]
	//RSLayoutType GetComputeRSLayout() const noexcept;
	[[nodiscard]]
	ID3D12Resource* GetArgumentBuffer(size_t frameIndex) const noexcept;
	[[nodiscard]]
	ID3D12Resource* GetCounterBuffer(size_t frameIndex) const noexcept;

private:
	/*
	[[nodiscard]]
	std::unique_ptr<RootSignatureDynamic> _createComputeRootSignature(
		ID3D12Device* device
	) const noexcept;
	*/
	[[nodiscard]]
	std::unique_ptr<D3DPipelineObject> _createComputePipelineObject(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const std::wstring& shaderPath
	) const noexcept;

private:
	//std::unique_ptr<RootSignatureBase> m_computeRS;
	std::unique_ptr<D3DPipelineObject> m_computePSO;
	//RSLayoutType m_computeRSLayout;

	D3DUploadResourceDescriptorView m_argumentBufferSRV;
	std::vector<D3DDescriptorView> m_argumentBufferUAVs;
	std::vector<D3DUploadResourceDescriptorView> m_counterBuffers;
	D3DResourceView m_counterResetBuffer;
	D3DUploadableResourceView m_cullingDataBuffer;
	std::vector<ModelDrawArguments> m_indirectArguments;
	std::vector<std::uint32_t> m_modelCountOffsets;
	UINT m_modelCount;
	std::uint32_t m_frameCount;

	static constexpr float THREADBLOCKSIZE = 64.f;
	static constexpr DirectX::XMFLOAT2 XBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 YBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 ZBOUNDS = { 1.f, -1.f };
	static constexpr UINT64 COUNTERBUFFERSTRIDE =
		static_cast<UINT64>(sizeof(std::uint32_t) * 2u);

private:
	struct CullingData {
		std::uint32_t modelCount;
		std::uint32_t modelTypes;
		DirectX::XMFLOAT2 xBounds;
		DirectX::XMFLOAT2 yBounds;
		DirectX::XMFLOAT2 zBounds;
	};
};
#endif
