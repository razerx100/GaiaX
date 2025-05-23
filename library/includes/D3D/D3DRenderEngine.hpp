#ifndef D3D_RENDER_ENGINE_
#define D3D_RENDER_ENGINE_
#include <D3DHeaders.hpp>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <D3DDeviceManager.hpp>
#include <D3DFence.hpp>
#include <TemporaryDataBuffer.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DSharedBuffer.hpp>
#include <D3DTextureManager.hpp>
#include <D3DCameraManager.hpp>
#include <D3DViewportAndScissorManager.hpp>
#include <D3DRootSignature.hpp>
#include <ModelBundle.hpp>
#include <Shader.hpp>
#include <MeshBundle.hpp>
#include <D3DModelBuffer.hpp>
#include <D3DPipelineManager.hpp>
#include <D3DExternalRenderPass.hpp>
#include <D3DExternalResourceManager.hpp>

namespace Gaia
{
class RenderEngine
{
	// Getting the values from the same values from the deviceManager for each member is kinda
	// dumb, so keeping this constructor but making it private.
	RenderEngine(
		IDXGIAdapter3* adapter, ID3D12Device5* device, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
	);

protected:
	using ExternalRenderPassSP_t        = std::shared_ptr<D3DExternalRenderPass>;
	using ExternalRenderPassContainer_t = Callisto::ReusableVector<ExternalRenderPassSP_t>;

public:
	RenderEngine(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
	);

	template<class Derived>
	[[nodiscard]]
	size_t AddTexture(this Derived& self, STexture&& texture)
	{
		self.WaitForGPUToFinish();

		const size_t textureIndex = self.m_textureStorage.AddTexture(
			std::move(texture), self.m_stagingManager, self.m_temporaryDataBuffer
		);

		self.m_gpuCopyNecessary = true;

		return textureIndex;
	}

	void UnbindTexture(size_t textureIndex, UINT bindingIndex);

	template<class Derived>
	[[nodiscard]]
	std::uint32_t BindTexture(this Derived& self, size_t textureIndex)
	{
		const Texture& texture = self.m_textureStorage.Get(textureIndex);

		// The current caching system only works for read only single textures which are bound to
		// multiple descriptor managers. Because we only cache one of them.
		std::optional<UINT> localCacheIndex
			= self.m_textureStorage.GetAndRemoveTextureLocalDescIndex(
				static_cast<UINT>(textureIndex)
			);

		return self.BindTextureCommon(texture, localCacheIndex);
	}

	void UnbindExternalTexture(UINT bindingIndex);

	void RebindExternalTexture(size_t textureIndex, UINT bindingIndex);

	template<class Derived>
	[[nodiscard]]
	std::uint32_t BindExternalTexture(this Derived& self, size_t textureIndex)
	{
		D3DExternalResourceFactory& resourceFactory
			= self.m_externalResourceManager.GetResourceFactory();

		const Texture& texture = resourceFactory.GetD3DTexture(textureIndex);

		// Can't cache as the underlying resource might change or we might have a separate texture
		// on each descriptor buffer.
		return self.BindTextureCommon(texture, {});
	}

	template<class Derived>
	void RemoveTexture(this Derived& self, size_t textureIndex)
	{
		self.WaitForGPUToFinish();

		std::vector<UINT> localTextureCacheIndices
			= self.m_textureStorage.GetAndRemoveTextureCacheDetails(
				static_cast<UINT>(textureIndex)
			);

		for (UINT localCacheIndex : localTextureCacheIndices)
			self.m_textureManager.SetLocalDescriptorAvailability<D3D12_DESCRIPTOR_RANGE_TYPE_SRV>(
				localCacheIndex, true
			);

		self.m_textureStorage.RemoveTexture(textureIndex);
	}

	[[nodiscard]]
	const D3DCommandQueue& GetPresentQueue() const noexcept { return m_graphicsQueue; }

private:
	template<class Derived>
	[[nodiscard]]
	UINT BindTextureCommon(
		this Derived& self, const Texture& texture, std::optional<UINT> oLocalCacheIndex
	) {
		self.WaitForGPUToFinish();

		static constexpr D3D12_DESCRIPTOR_RANGE_TYPE DescType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

		std::optional<size_t> oFreeGlobalDescIndex
			= self.m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

		// If there is no free global index, increase the limit. Right now it should be possible to
		// have 65535 bound textures at once. There could be more textures.
		if (!oFreeGlobalDescIndex)
		{
			self.m_textureManager.IncreaseMaximumBindingCount<DescType>();

			for (D3DDescriptorManager& descriptorManager : self.m_graphicsDescriptorManagers)
			{
				const std::vector<D3DDescriptorLayout> oldLayouts
					= descriptorManager.GetLayouts();

				self.m_textureManager.SetDescriptorLayout(
					descriptorManager, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
				);

				descriptorManager.RecreateDescriptors(oldLayouts);
			}

			oFreeGlobalDescIndex = self.m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

			// Don't need to reset the pipelines, since the table was created as bindless.
		}

		const auto freeGlobalDescIndex = static_cast<UINT>(oFreeGlobalDescIndex.value());

		self.m_textureManager.SetBindingAvailability<DescType>(freeGlobalDescIndex, false);

		if (oLocalCacheIndex)
		{
			const UINT localCacheIndex = oLocalCacheIndex.value();

			const D3D12_CPU_DESCRIPTOR_HANDLE localDescriptor
				= self.m_textureManager.GetLocalDescriptor<DescType>(localCacheIndex);

			self.m_textureManager.SetLocalDescriptorAvailability<DescType>(localCacheIndex, true);

			for (D3DDescriptorManager& descriptorManager : self.m_graphicsDescriptorManagers)
				descriptorManager.SetDescriptorSRV(
					localDescriptor, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace,
					freeGlobalDescIndex
				);
		}
		else
			for (D3DDescriptorManager& descriptorManager : self.m_graphicsDescriptorManagers)
				descriptorManager.CreateSRV(
					s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace, freeGlobalDescIndex,
					texture.Get(), texture.GetSRVDesc()
				);
		// Since it is a descriptor table, there is no point in setting it every time.
		// It should be fine to just bind it once after the descriptorManagers have
		// been created.

		return freeGlobalDescIndex;
	}

public:
	// External stuff
	[[nodiscard]]
	auto&& GetExternalResourceManager(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(self.m_externalResourceManager);
	}

	void UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails);

	void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData,
		size_t srcDataSizeInBytes, size_t dstBufferOffset
	);
	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
	);

	[[nodiscard]]
	std::uint32_t AddExternalRenderPass(D3DReusableDescriptorHeap* rtvHeap)
	{
		return static_cast<std::uint32_t>(
			m_renderPasses.Add(
				std::make_shared<D3DExternalRenderPass>(rtvHeap, m_dsvHeap.get())
			)
		);
	}

	[[nodiscard]]
	D3DExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept
	{
		return m_renderPasses[index].get();
	}

	[[nodiscard]]
	std::shared_ptr<D3DExternalRenderPass> GetExternalRenderPassSP(size_t index) const noexcept
	{
		return m_renderPasses[index];
	}

	void RemoveExternalRenderPass(size_t index) noexcept
	{
		m_renderPasses[index].reset();
		m_renderPasses.RemoveElement(index);
	}

	void SetSwapchainExternalRenderPass(D3DReusableDescriptorHeap* rtvHeap)
	{
		m_swapchainRenderPass = std::make_shared<D3DExternalRenderPass>(rtvHeap, m_dsvHeap.get());
	}

	[[nodiscard]]
	D3DExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept
	{
		return m_swapchainRenderPass.get();
	}

	[[nodiscard]]
	std::shared_ptr<D3DExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept
	{
		return m_swapchainRenderPass;
	}

	void RemoveSwapchainExternalRenderPass() noexcept
	{
		m_swapchainRenderPass.reset();
	}

	[[nodiscard]]
	size_t GetActiveRenderPassCount() const noexcept
	{
		size_t activeRenderPassCount = m_renderPasses.GetIndicesManager().GetActiveIndexCount();

		if (m_swapchainRenderPass)
			++activeRenderPassCount;

		return activeRenderPassCount;
	}

protected:
	void WaitForGraphicsQueueToFinish();

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

	static constexpr size_t s_textureSRVRegisterSlot = 1u;

protected:
	std::shared_ptr<ThreadPool>                m_threadPool;
	std::unique_ptr<MemoryManager>             m_memoryManager;
	std::vector<UINT64>                        m_counterValues;
	D3DCommandQueue                            m_graphicsQueue;
	std::vector<D3DFence>                      m_graphicsWait;
	D3DCommandQueue                            m_copyQueue;
	std::vector<D3DFence>                      m_copyWait;
	StagingBufferManager                       m_stagingManager;
	std::unique_ptr<D3DReusableDescriptorHeap> m_dsvHeap;
	std::vector<D3DDescriptorManager>          m_graphicsDescriptorManagers;
	D3DExternalResourceManager                 m_externalResourceManager;
	D3DRootSignature                           m_graphicsRootSignature;
	TextureStorage                             m_textureStorage;
	TextureManager                             m_textureManager;
	CameraManager                              m_cameraManager;
	ViewportAndScissorManager                  m_viewportAndScissors;
	Callisto::TemporaryDataBufferGPU           m_temporaryDataBuffer;
	ExternalRenderPassContainer_t              m_renderPasses;
	ExternalRenderPassSP_t                     m_swapchainRenderPass;
	bool                                       m_gpuCopyNecessary;

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
		m_renderPasses{ std::move(other.m_renderPasses) },
		m_swapchainRenderPass{ std::move(other.m_swapchainRenderPass) },
		m_gpuCopyNecessary{ other.m_gpuCopyNecessary }
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
		m_renderPasses               = std::move(other.m_renderPasses);
		m_swapchainRenderPass        = std::move(other.m_swapchainRenderPass);
		m_gpuCopyNecessary           = other.m_gpuCopyNecessary;

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
public:
	RenderEngineCommon(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
	) : RenderEngine{ deviceManager, std::move(threadPool), frameCount },
		m_modelManager{
			Derived::CreateModelManager(
				deviceManager.GetDevice(), m_memoryManager.get(),
				static_cast<std::uint32_t>(frameCount)
			)
		},
		m_modelBuffers{
			deviceManager.GetDevice(), m_memoryManager.get(),
			static_cast<std::uint32_t>(frameCount)
		},
		m_meshManager{ deviceManager.GetDevice(), m_memoryManager.get() },
		m_graphicsPipelineManager{ deviceManager.GetDevice() }
	{
		for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
			m_textureManager.SetDescriptorLayout(
				descriptorManager, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
			);
	}

	void SetModelContainer(std::shared_ptr<ModelContainer> modelContainer) noexcept
	{
		m_modelBuffers.SetModelContainer(std::move(modelContainer));
	}

	[[nodiscard]]
	std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline)
	{
		return m_graphicsPipelineManager.AddOrGetGraphicsPipeline(gfxPipeline);
	}

	void ReconfigureModelPipelinesInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t decreasedModelsPipelineIndex,
		std::uint32_t increasedModelsPipelineIndex
	) {
		m_modelManager.ReconfigureModels(
			modelBundleIndex, decreasedModelsPipelineIndex, increasedModelsPipelineIndex
		);
	}

	void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept
	{
		m_graphicsPipelineManager.SetOverwritable(pipelineIndex);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
	{
		m_meshManager.RemoveMeshBundle(bundleIndex);
	}

	[[nodiscard]]
	std::shared_ptr<ModelBundle> RemoveModelBundle(std::uint32_t bundleIndex) noexcept
	{
		return m_modelManager.RemoveModelBundle(bundleIndex);
	}

	void Resize(UINT width, UINT height)
	{
		m_viewportAndScissors.Resize(width, height);
	}

	void WaitForCurrentBackBuffer(size_t frameIndex)
	{
		// Wait for the previous Graphics command buffer to finish.
		const UINT64 counterValue = m_counterValues[frameIndex];

		m_graphicsWait[frameIndex].Wait(counterValue);
		// It should be okay to clear the data now that the frame has finished
		// its submission.
		m_temporaryDataBuffer.Clear(frameIndex);
	}

	void UpdateCamera(size_t frameIndex, const Camera& cameraData) const noexcept
	{
		m_cameraManager.Update(static_cast<UINT64>(frameIndex), cameraData);
	}

	void Update(size_t frameIndex) const noexcept
	{
		static_cast<Derived const*>(this)->_updatePerFrame(static_cast<UINT64>(frameIndex));
	}

	void Render(size_t frameIndex, ID3D12Resource* swapchainBackBuffer)
	{
		UINT64& counterValue   = m_counterValues[frameIndex];
		// Passing this as the wait fence is kinda useless, but to keep
		// all the pipelineStage function signature the same, gonna pass it
		// as it should immedietly return.
		ID3D12Fence* waitFence = m_graphicsWait[frameIndex].Get();

		static_cast<Derived*>(this)->ExecutePipelineStages(
			frameIndex, swapchainBackBuffer, counterValue, waitFence
		);
	}

	void AddLocalPipelinesInExternalRenderPass(
		std::uint32_t modelBundleIndex, size_t renderPassIndex
	) {
		// UINT32 MAX because I am not gonna store size_t for no reason.
		// As there is no way there will be 4 million passes.
		if (renderPassIndex == std::numeric_limits<std::uint32_t>::max())
			m_swapchainRenderPass->AddLocalPipelinesOfModelBundle(
				modelBundleIndex, m_modelManager
			);
		else
			m_renderPasses[renderPassIndex]->AddLocalPipelinesOfModelBundle(
				modelBundleIndex, m_modelManager
			);
	}

protected:
	void _setShaderPath(const std::wstring& shaderPath)
	{
		m_graphicsPipelineManager.SetShaderPath(shaderPath);
	}

	void SetCommonGraphicsDescriptorLayout(
		D3D12_SHADER_VISIBILITY cameraShaderVisibility
	) noexcept {
		m_cameraManager.SetDescriptorLayoutGraphics(
			m_graphicsDescriptorManagers, Derived::s_cameraCBVRegisterSlot,
			s_vertexShaderRegisterSpace, cameraShaderVisibility
		);
	}

protected:
	ModelManager_t                      m_modelManager;
	ModelBuffers                        m_modelBuffers;
	MeshManager_t                       m_meshManager;
	PipelineManager<GraphicsPipeline_t> m_graphicsPipelineManager;

public:
	RenderEngineCommon(const RenderEngineCommon&) = delete;
	RenderEngineCommon& operator=(const RenderEngineCommon&) = delete;

	RenderEngineCommon(RenderEngineCommon&& other) noexcept
		: RenderEngine{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) },
		m_modelBuffers{ std::move(other.m_modelBuffers) },
		m_meshManager{ std::move(other.m_meshManager) },
		m_graphicsPipelineManager{ std::move(other.m_graphicsPipelineManager) }
	{}
	RenderEngineCommon& operator=(RenderEngineCommon&& other) noexcept
	{
		RenderEngine::operator=(std::move(other));
		m_modelManager            = std::move(other.m_modelManager);
		m_modelBuffers            = std::move(other.m_modelBuffers);
		m_meshManager             = std::move(other.m_meshManager);
		m_graphicsPipelineManager = std::move(other.m_graphicsPipelineManager);

		return *this;
	}
};
}
#endif
