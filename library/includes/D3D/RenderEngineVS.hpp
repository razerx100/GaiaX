#ifndef RENDER_ENGINE_VS_HPP_
#define RENDER_ENGINE_VS_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

class RenderEngineVSIndividual : public
	RenderEngineCommon
	<
	MeshManagerVSIndividual,
	GraphicsPipelineVSIndividualDraw,
	RenderEngineVSIndividual
	>
{
	friend class RenderEngineCommon
		<
		MeshManagerVSIndividual,
		GraphicsPipelineVSIndividualDraw,
		RenderEngineVSIndividual
		>;

public:
	RenderEngineVSIndividual(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	void FinaliseInitialisation(const DeviceManager& deviceManager) override;

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
	) override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override;

private:
	void ExecutePipelineStages(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue,
		ID3D12Fence* waitFence
	);

	[[nodiscard]]
	ID3D12Fence* GenericCopyStage(
		size_t frameIndex, UINT64& counterValue, ID3D12Fence* waitFence
	);
	void DrawingStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
	);

	void SetGraphicsDescriptorBufferLayout();
	void SetGraphicsDescriptors();

	void _updatePerFrame(UINT64 frameIndex) const noexcept
	{
		m_modelBuffers.Update(frameIndex);
	}

private:
	ModelManagerVSIndividual m_modelManager;
	ModelBuffers             m_modelBuffers;

public:
	RenderEngineVSIndividual(const RenderEngineVSIndividual&) = delete;
	RenderEngineVSIndividual& operator=(const RenderEngineVSIndividual&) = delete;

	RenderEngineVSIndividual(RenderEngineVSIndividual&& other) noexcept
		: RenderEngineCommon{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) },
		m_modelBuffers{ std::move(other.m_modelBuffers) }
	{}
	RenderEngineVSIndividual& operator=(RenderEngineVSIndividual&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));
		m_modelManager = std::move(other.m_modelManager);
		m_modelBuffers = std::move(other.m_modelBuffers);

		return *this;
	}
};

class RenderEngineVSIndirect : public
	RenderEngineCommon
	<
		MeshManagerVSIndirect,
		GraphicsPipelineVSIndirectDraw,
		RenderEngineVSIndirect
	>
{
	friend class RenderEngineCommon
		<
			MeshManagerVSIndirect,
			GraphicsPipelineVSIndirectDraw,
			RenderEngineVSIndirect
		>;

	using ComputePipeline_t = ComputePipeline;

public:
	RenderEngineVSIndirect(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	void FinaliseInitialisation(const DeviceManager& deviceManager) override;

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
	) override;

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

	void WaitForGPUToFinish() override;

	void SetShaderPath(const std::wstring& shaderPath) override;

private:
	void ExecutePipelineStages(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue,
		ID3D12Fence* waitFence
	);

	[[nodiscard]]
	ID3D12Fence* GenericCopyStage(
		size_t frameIndex, UINT64& counterValue, ID3D12Fence* waitFence
	);
	[[nodiscard]]
	ID3D12Fence* FrustumCullingStage(
		size_t frameIndex, UINT64& counterValue, ID3D12Fence* waitFence
	);
	void DrawingStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
	);

	void SetGraphicsDescriptorBufferLayout();
	void SetModelGraphicsDescriptors();

	void SetComputeDescriptorBufferLayout();
	void SetModelComputeDescriptors();

	void CreateCommandSignature(ID3D12Device* device);

	void _updatePerFrame(UINT64 frameIndex) const noexcept;

private:
	// Compute
	static constexpr size_t s_computePipelineSetLayoutCount = 1u;
	static constexpr size_t s_computeShaderRegisterSpace    = 0u;

	static constexpr size_t s_modelBuffersCSSRVRegisterSlot = 0u;
	static constexpr size_t s_cameraCSCBVRegisterSlot       = 1u;

private:
	ModelManagerVSIndirect             m_modelManager;
	ModelBuffers                       m_modelBuffers;
	D3DCommandQueue                    m_computeQueue;
	std::vector<D3DFence>              m_computeWait;
	std::vector<D3DDescriptorManager>  m_computeDescriptorManagers;
	PipelineManager<ComputePipeline_t> m_computePipelineManager;
	D3DRootSignature                   m_computeRootSignature;
	ComPtr<ID3D12CommandSignature>     m_commandSignature;

public:
	RenderEngineVSIndirect(const RenderEngineVSIndirect&) = delete;
	RenderEngineVSIndirect& operator=(const RenderEngineVSIndirect&) = delete;

	RenderEngineVSIndirect(RenderEngineVSIndirect&& other) noexcept
		: RenderEngineCommon{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) },
		m_modelBuffers{ std::move(other.m_modelBuffers) },
		m_computeQueue{ std::move(other.m_computeQueue) },
		m_computeWait{ std::move(other.m_computeWait) },
		m_computeDescriptorManagers{ std::move(other.m_computeDescriptorManagers) },
		m_computePipelineManager{ std::move(other.m_computePipelineManager) },
		m_computeRootSignature{ std::move(other.m_computeRootSignature) },
		m_commandSignature{ std::move(other.m_commandSignature) }
	{}
	RenderEngineVSIndirect& operator=(RenderEngineVSIndirect&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));
		m_modelManager              = std::move(other.m_modelManager);
		m_modelBuffers              = std::move(other.m_modelBuffers);
		m_computeQueue              = std::move(other.m_computeQueue);
		m_computeWait               = std::move(other.m_computeWait);
		m_computeDescriptorManagers = std::move(other.m_computeDescriptorManagers);
		m_computePipelineManager    = std::move(other.m_computePipelineManager);
		m_computeRootSignature      = std::move(other.m_computeRootSignature);
		m_commandSignature          = std::move(other.m_commandSignature);

		return *this;
	}
};
#endif
