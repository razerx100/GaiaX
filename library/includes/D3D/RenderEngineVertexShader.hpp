#ifndef RENDER_ENGINE_VERTEX_SHADER_HPP_
#define RENDER_ENGINE_VERTEX_SHADER_HPP_
#include <RenderEngine.hpp>
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
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& pixelShader
	) override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;

private:
	[[nodiscard]]
	static ModelManagerVSIndividual GetModelManager(
		const DeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

	[[nodiscard]]
	ID3D12Fence* GenericCopyStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
	);
	[[nodiscard]]
	ID3D12Fence* DrawingStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
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

class RenderEngineVSIndirect
	: public RenderEngineCommon<ModelManagerVSIndirect, RenderEngineVSIndirect>
{
	friend class RenderEngineCommon<ModelManagerVSIndirect, RenderEngineVSIndirect>;

public:
	RenderEngineVSIndirect(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& pixelShader
	) override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;

private:
	[[nodiscard]]
	static ModelManagerVSIndirect GetModelManager(
		const DeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

	[[nodiscard]]
	ID3D12Fence* GenericCopyStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
	);
	[[nodiscard]]
	ID3D12Fence* FrustumCullingStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
	);
	[[nodiscard]]
	ID3D12Fence* DrawingStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
	);

	void SetupPipelineStages();

private:
	// Graphics
	static constexpr size_t s_cameraRegisterSlot = 2u;

	// Compute
	static constexpr size_t s_computePipelineSetLayoutCount = 1u;
	static constexpr size_t s_computeShaderRegisterSpace    = 0u;

	static constexpr std::uint32_t s_cameraComputeRegisterSlot = 11u;

private:
	D3DCommandQueue                   m_computeQueue;
	std::vector<D3DFence>             m_computeWait;
	std::vector<D3DDescriptorManager> m_computeDescriptorManagers;

public:
	RenderEngineVSIndirect(const RenderEngineVSIndirect&) = delete;
	RenderEngineVSIndirect& operator=(const RenderEngineVSIndirect&) = delete;

	RenderEngineVSIndirect(RenderEngineVSIndirect&& other) noexcept
		: RenderEngineCommon{ std::move(other) },
		m_computeQueue{ std::move(other.m_computeQueue) },
		m_computeWait{ std::move(other.m_computeWait) },
		m_computeDescriptorManagers{ std::move(other.m_computeDescriptorManagers) }
	{}
	RenderEngineVSIndirect& operator=(RenderEngineVSIndirect&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));
		m_computeQueue              = std::move(other.m_computeQueue);
		m_computeWait               = std::move(other.m_computeWait);
		m_computeDescriptorManagers = std::move(other.m_computeDescriptorManagers);

		return *this;
	}
};
#endif
