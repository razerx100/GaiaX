#ifndef RENDER_ENGINE_
#define RENDER_ENGINE_
#include <D3DHeaders.hpp>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <DeviceManager.hpp>
#include <D3DFence.hpp>
#include <TemporaryDataBuffer.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DSharedBuffer.hpp>
#include <TextureManager.hpp>
#include <CameraManager.hpp>
#include <ViewportAndScissorManager.hpp>
#include <D3DRootSignature.hpp>
#include <Model.hpp>
#include <Shader.hpp>
#include <MeshBundle.hpp>
#include <D3DModelBuffer.hpp>
#include <PipelineManager.hpp>
#include <D3DExternalRenderPass.hpp>
#include <D3DExternalResourceManager.hpp>

class RenderEngine
{
	// Getting the values from the same values from the deviceManager for each member is kinda
	// dumb, so keeping this constructor but making it private.
	RenderEngine(
		IDXGIAdapter3* adapter, ID3D12Device5* device, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
	);

public:
	RenderEngine(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);
	virtual ~RenderEngine() = default;

	virtual void FinaliseInitialisation(const DeviceManager& deviceManager) = 0;

	[[nodiscard]]
	size_t AddTexture(STexture&& texture);

	void UnbindTexture(size_t textureIndex, UINT bindingIndex);
	[[nodiscard]]
	std::uint32_t BindTexture(size_t textureIndex);

	void UnbindExternalTexture(UINT bindingIndex);

	void RebindExternalTexture(size_t textureIndex, UINT bindingIndex);

	[[nodiscard]]
	std::uint32_t BindExternalTexture(size_t textureIndex);

	void RemoveTexture(size_t textureIndex);

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera> camera) noexcept
	{
		return m_cameraManager.AddCamera(std::move(camera));
	}
	void SetCamera(std::uint32_t index) noexcept { m_cameraManager.SetCamera(index); }
	void RemoveCamera(std::uint32_t index) noexcept { m_cameraManager.RemoveCamera(index); }

	virtual void SetShaderPath(const std::wstring& shaderPath) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline) = 0;

	virtual void ChangeModelPipelineInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
		std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) = 0;
	virtual void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept = 0;

	[[nodiscard]]
	virtual void Render(size_t frameIndex, ID3D12Resource* swapchainBackBuffer) = 0;
	virtual void Resize(UINT width, UINT height) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) = 0;

	virtual void RemoveModelBundle(std::uint32_t bundleID) noexcept = 0;

	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) = 0;

	virtual void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept = 0;

	// Making this function, so this can be overriden to add the compute queues in
	// certain Engines.
	virtual void WaitForGPUToFinish();

	[[nodiscard]]
	const D3DCommandQueue& GetPresentQueue() const noexcept { return m_graphicsQueue; }

private:
	[[nodiscard]]
	UINT BindTextureCommon(const Texture& texture, std::optional<UINT> oLocalCacheIndex);

public:
	// External stuff
	[[nodiscard]]
	ExternalResourceManager* GetExternalResourceManager() noexcept
	{
		return m_externalResourceManager.get();
	}

	void UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails);

	void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
		size_t dstBufferOffset
	);
	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
	);

	[[nodiscard]]
	virtual std::uint32_t AddExternalRenderPass(D3DReusableDescriptorHeap* rtvHeap) = 0;
	[[nodiscard]]
	virtual ExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept = 0;
	[[nodiscard]]
	virtual std::shared_ptr<ExternalRenderPass> GetExternalRenderPassSP(
		size_t index
	) const noexcept = 0;

	virtual void SetSwapchainExternalRenderPass(D3DReusableDescriptorHeap* rtvHeap) = 0;

	[[nodiscard]]
	virtual ExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept = 0;
	[[nodiscard]]
	virtual std::shared_ptr<ExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept = 0;

	virtual void RemoveExternalRenderPass(size_t index) noexcept = 0;
	virtual void RemoveSwapchainExternalRenderPass() noexcept = 0;

	[[nodiscard]]
	virtual size_t GetActiveRenderPassCount() const noexcept = 0;

protected:
	[[nodiscard]]
	virtual size_t GetCameraRegisterSlot() const noexcept = 0;

	[[nodiscard]]
	static std::vector<std::uint32_t> AddModelsToBuffer(
		const ModelBundle& modelBundle, ModelBuffers& modelBuffers
	) noexcept;

	void SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY cameraShaderVisibility) noexcept;

protected:
	// These descriptors are bound to the pixel shader. So, they should be the same across
	// all of the pipeline types. That's why we are going to bind them to their own RegisterSpace.
	static constexpr size_t s_graphicsPipelineSetLayoutCount = 3u;
	static constexpr size_t s_vertexShaderRegisterSpace      = 0u;
	static constexpr size_t s_pixelShaderRegisterSpace       = 1u;

	// Space 0
	static constexpr size_t s_modelBuffersGraphicsSRVRegisterSlot = 0u;
	static constexpr size_t s_cameraCBVRegisterSlot               = 1u;

	// Space 1
	static constexpr size_t s_modelBuffersPixelSRVRegisterSlot    = 0u;

	static constexpr size_t s_textureSRVRegisterSlot  = 1u;

protected:
	std::shared_ptr<ThreadPool>                 m_threadPool;
	std::unique_ptr<MemoryManager>              m_memoryManager;
	std::vector<UINT64>                         m_counterValues;
	D3DCommandQueue                             m_graphicsQueue;
	std::vector<D3DFence>                       m_graphicsWait;
	D3DCommandQueue                             m_copyQueue;
	std::vector<D3DFence>                       m_copyWait;
	StagingBufferManager                        m_stagingManager;
	std::unique_ptr<D3DReusableDescriptorHeap>  m_dsvHeap;
	std::vector<D3DDescriptorManager>           m_graphicsDescriptorManagers;
	std::unique_ptr<D3DExternalResourceManager> m_externalResourceManager;
	D3DRootSignature                            m_graphicsRootSignature;
	TextureStorage                              m_textureStorage;
	TextureManager                              m_textureManager;
	CameraManager                               m_cameraManager;
	ViewportAndScissorManager                   m_viewportAndScissors;
	TemporaryDataBufferGPU                      m_temporaryDataBuffer;
	bool                                        m_copyNecessary;

public:
	RenderEngine(const RenderEngine&) = delete;
	RenderEngine& operator=(const RenderEngine&) = delete;

	RenderEngine(RenderEngine&& other) noexcept
		: m_threadPool{ std::move(other.m_threadPool) },
		m_memoryManager{ std::move(other.m_memoryManager) },
		m_counterValues{ std::move(other.m_counterValues) },
		m_graphicsQueue{ std::move(other.m_graphicsQueue) },
		m_graphicsWait{ std::move(other.m_graphicsWait) },
		m_copyQueue{ std::move(other.m_copyQueue) },
		m_copyWait{ std::move(other.m_copyWait) },
		m_stagingManager{ std::move(other.m_stagingManager) },
		m_dsvHeap{ std::move(other.m_dsvHeap) },
		m_graphicsDescriptorManagers{ std::move(other.m_graphicsDescriptorManagers) },
		m_externalResourceManager{ std::move(other.m_externalResourceManager) },
		m_graphicsRootSignature{ std::move(other.m_graphicsRootSignature) },
		m_textureStorage{ std::move(other.m_textureStorage) },
		m_textureManager{ std::move(other.m_textureManager) },
		m_cameraManager{ std::move(other.m_cameraManager) },
		m_viewportAndScissors{ other.m_viewportAndScissors },
		m_temporaryDataBuffer{ std::move(other.m_temporaryDataBuffer) },
		m_copyNecessary{ other.m_copyNecessary }
	{}
	RenderEngine& operator=(RenderEngine&& other) noexcept
	{
		m_threadPool                 = std::move(other.m_threadPool);
		m_memoryManager              = std::move(other.m_memoryManager);
		m_counterValues              = std::move(other.m_counterValues);
		m_graphicsQueue              = std::move(other.m_graphicsQueue);
		m_graphicsWait               = std::move(other.m_graphicsWait);
		m_copyQueue                  = std::move(other.m_copyQueue);
		m_copyWait                   = std::move(other.m_copyWait);
		m_stagingManager             = std::move(other.m_stagingManager);
		m_dsvHeap                    = std::move(other.m_dsvHeap);
		m_graphicsDescriptorManagers = std::move(other.m_graphicsDescriptorManagers);
		m_externalResourceManager    = std::move(other.m_externalResourceManager);
		m_graphicsRootSignature      = std::move(other.m_graphicsRootSignature);
		m_textureStorage             = std::move(other.m_textureStorage);
		m_textureManager             = std::move(other.m_textureManager);
		m_cameraManager              = std::move(other.m_cameraManager);
		m_viewportAndScissors        = other.m_viewportAndScissors;
		m_temporaryDataBuffer        = std::move(other.m_temporaryDataBuffer);
		m_copyNecessary              = other.m_copyNecessary;

		return *this;
	}
};

template<
	typename ModelManager_t,
	typename MeshManager_t,
	typename GraphicsPipeline_t,
	typename Derived
>
class RenderEngineCommon : public RenderEngine
{
protected:
	using ExternalRenderPass_t   = D3DExternalRenderPassCommon<ModelManager_t>;
	using ExternalRenderPassSP_t = std::shared_ptr<ExternalRenderPass_t>;

public:
	RenderEngineCommon(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	) : RenderEngine{ deviceManager, std::move(threadPool), frameCount },
		m_modelManager{},
		m_modelBuffers{
			deviceManager.GetDevice(), m_memoryManager.get(), static_cast<std::uint32_t>(frameCount)
		},
		m_meshManager{ deviceManager.GetDevice(), m_memoryManager.get() },
		m_graphicsPipelineManager{ deviceManager.GetDevice() },
		m_renderPasses{}, m_swapchainRenderPass{}
	{
		for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
			m_textureManager.SetDescriptorLayout(
				descriptorManager, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
			);
	}

	void SetShaderPath(const std::wstring& shaderPath) override
	{
		m_graphicsPipelineManager.SetShaderPath(shaderPath);
	}
	[[nodiscard]]
	std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline) override
	{
		return m_graphicsPipelineManager.AddOrGetGraphicsPipeline(gfxPipeline);
	}

	void ChangeModelPipelineInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
		std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) override {
		m_modelManager->ChangeModelPipeline(
			modelBundleIndex, modelIndex, oldPipelineIndex, newPipelineIndex
		);
	}

	void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept override
	{
		m_graphicsPipelineManager.SetOverwritable(pipelineIndex);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_meshManager.RemoveMeshBundle(bundleIndex);
	}

	void Resize(UINT width, UINT height) override
	{
		m_viewportAndScissors.Resize(width, height);
	}

	[[nodiscard]]
	size_t GetCameraRegisterSlot() const noexcept override
	{
		return Derived::s_cameraCBVRegisterSlot;
	}

	void Render(size_t frameIndex, ID3D12Resource* swapchainBackBuffer) final
	{
		// Wait for the previous Graphics command buffer to finish.
		UINT64& counterValue = m_counterValues[frameIndex];

		m_graphicsWait[frameIndex].Wait(counterValue);
		// It should be okay to clear the data now that the frame has finished
		// its submission.
		m_temporaryDataBuffer.Clear(frameIndex);

		Update(static_cast<UINT64>(frameIndex));

		// Passing this as the wait fence is kinda useless, but to keep
		// all the pipelineStage function signature the same, gonna pass it
		// as it should immedietly return.
		ID3D12Fence* waitFence = m_graphicsWait[frameIndex].Get();

		static_cast<Derived*>(this)->ExecutePipelineStages(
			frameIndex, swapchainBackBuffer, counterValue, waitFence
		);
	}

	[[nodiscard]]
	std::uint32_t AddExternalRenderPass(D3DReusableDescriptorHeap* rtvHeap) override
	{
		return static_cast<std::uint32_t>(
			m_renderPasses.Add(
				std::make_shared<ExternalRenderPass_t>(
					m_modelManager.get(), m_externalResourceManager->GetD3DResourceFactory(),
					rtvHeap, m_dsvHeap.get()
				)
			)
		);
	}

	[[nodiscard]]
	ExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept override
	{
		return m_renderPasses[index].get();
	}

	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetExternalRenderPassSP(size_t index) const noexcept override
	{
		return m_renderPasses[index];
	}

	void RemoveExternalRenderPass(size_t index) noexcept override
	{
		m_renderPasses[index].reset();
		m_renderPasses.RemoveElement(index);
	}

	void SetSwapchainExternalRenderPass(D3DReusableDescriptorHeap* rtvHeap) override
	{
		m_swapchainRenderPass = std::make_shared<ExternalRenderPass_t>(
			m_modelManager.get(), m_externalResourceManager->GetD3DResourceFactory(),
			rtvHeap, m_dsvHeap.get()
		);
	}

	[[nodiscard]]
	ExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept override
	{
		return m_swapchainRenderPass.get();
	}

	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept override
	{
		return m_swapchainRenderPass;
	}

	void RemoveSwapchainExternalRenderPass() noexcept override
	{
		m_swapchainRenderPass.reset();
	}

	[[nodiscard]]
	size_t GetActiveRenderPassCount() const noexcept override
	{
		size_t activeRenderPassCount = m_renderPasses.GetIndicesManager().GetActiveIndexCount();

		if (m_swapchainRenderPass)
			++activeRenderPassCount;

		return activeRenderPassCount;
	}

protected:
	void Update(UINT64 frameIndex) const noexcept
	{
		m_cameraManager.Update(frameIndex);

		static_cast<Derived const*>(this)->_updatePerFrame(frameIndex);

		m_externalResourceManager->UpdateExtensionData(static_cast<size_t>(frameIndex));
	}

protected:
	std::unique_ptr<ModelManager_t>        m_modelManager;
	ModelBuffers                           m_modelBuffers;
	MeshManager_t                          m_meshManager;
	PipelineManager<GraphicsPipeline_t>    m_graphicsPipelineManager;
	ReusableVector<ExternalRenderPassSP_t> m_renderPasses;
	ExternalRenderPassSP_t                 m_swapchainRenderPass;

public:
	RenderEngineCommon(const RenderEngineCommon&) = delete;
	RenderEngineCommon& operator=(const RenderEngineCommon&) = delete;

	RenderEngineCommon(RenderEngineCommon&& other) noexcept
		: RenderEngine{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) },
		m_modelBuffers{ std::move(other.m_modelBuffers) },
		m_meshManager{ std::move(other.m_meshManager) },
		m_graphicsPipelineManager{ std::move(other.m_graphicsPipelineManager) },
		m_renderPasses{ std::move(other.m_renderPasses) },
		m_swapchainRenderPass{ std::move(other.m_swapchainRenderPass) }
	{}
	RenderEngineCommon& operator=(RenderEngineCommon&& other) noexcept
	{
		RenderEngine::operator=(std::move(other));
		m_modelManager            = std::move(other.m_modelManager);
		m_modelBuffers            = std::move(other.m_modelBuffers);
		m_meshManager             = std::move(other.m_meshManager);
		m_graphicsPipelineManager = std::move(other.m_graphicsPipelineManager);
		m_renderPasses            = std::move(other.m_renderPasses);
		m_swapchainRenderPass     = std::move(other.m_swapchainRenderPass);

		return *this;
	}
};
#endif
