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
#include <D3DRenderTarget.hpp>
#include <Model.hpp>
#include <Shader.hpp>
#include <MeshBundle.hpp>
#include <D3DModelBuffer.hpp>
#include <D3DRenderPassManager.hpp>
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

	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept;

	[[nodiscard]]
	size_t AddTexture(STexture&& texture);

	void UnbindTexture(size_t textureIndex);
	[[nodiscard]]
	std::uint32_t BindTexture(size_t textureIndex);

	void RemoveTexture(size_t index);

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera> camera) noexcept
	{
		return m_cameraManager.AddCamera(std::move(camera));
	}
	void SetCamera(std::uint32_t index) noexcept { m_cameraManager.SetCamera(index); }
	void RemoveCamera(std::uint32_t index) noexcept { m_cameraManager.RemoveCamera(index); }

	virtual void SetSwapchainRTVFormat(DXGI_FORMAT rtvFormat) = 0;
	virtual void SetShaderPath(const std::wstring& shaderPath) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddGraphicsPipeline(const ShaderName& pixelShader) = 0;

	virtual void ChangePixelShader(
		[[maybe_unused]] std::uint32_t modelBundleID,
		[[maybe_unused]] const ShaderName& pixelShader
	) {}
	virtual void MakePixelShaderRemovable(const ShaderName& pixelShader) noexcept = 0;

	[[nodiscard]]
	virtual void Render(size_t frameIndex, const RenderTarget& renderTarget) = 0;
	virtual void Resize(UINT width, UINT height) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
	) = 0;

	virtual void RemoveModelBundle(std::uint32_t bundleID) noexcept = 0;

	[[nodiscard]]
	ExternalResourceManager* GetExternalResourceManager() noexcept
	{
		return &m_externalResourceManager;
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
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) = 0;

	virtual void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept = 0;

	// Making this function, so this can be overriden to add the compute queues in
	// certain Engines.
	virtual void WaitForGPUToFinish();

	[[nodiscard]]
	const D3DCommandQueue& GetPresentQueue() const noexcept { return m_graphicsQueue; }

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
	std::shared_ptr<ThreadPool>       m_threadPool;
	MemoryManager                     m_memoryManager;
	std::vector<UINT64>               m_counterValues;
	D3DCommandQueue                   m_graphicsQueue;
	std::vector<D3DFence>             m_graphicsWait;
	D3DCommandQueue                   m_copyQueue;
	std::vector<D3DFence>             m_copyWait;
	StagingBufferManager              m_stagingManager;
	D3DExternalResourceManager        m_externalResourceManager;
	std::vector<D3DDescriptorManager> m_graphicsDescriptorManagers;
	D3DRootSignature                  m_graphicsRootSignature;
	TextureStorage                    m_textureStorage;
	TextureManager                    m_textureManager;
	CameraManager                     m_cameraManager;
	D3DReusableDescriptorHeap         m_dsvHeap;
	std::array<float, 4u>             m_backgroundColour;
	ViewportAndScissorManager         m_viewportAndScissors;
	TemporaryDataBufferGPU            m_temporaryDataBuffer;
	bool                              m_copyNecessary;

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
		m_externalResourceManager{ std::move(other.m_externalResourceManager) },
		m_graphicsDescriptorManagers{ std::move(other.m_graphicsDescriptorManagers) },
		m_graphicsRootSignature{ std::move(other.m_graphicsRootSignature) },
		m_textureStorage{ std::move(other.m_textureStorage) },
		m_textureManager{ std::move(other.m_textureManager) },
		m_cameraManager{ std::move(other.m_cameraManager) },
		m_dsvHeap{ std::move(other.m_dsvHeap) },
		m_backgroundColour{ other.m_backgroundColour },
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
		m_externalResourceManager    = std::move(other.m_externalResourceManager);
		m_graphicsDescriptorManagers = std::move(other.m_graphicsDescriptorManagers);
		m_graphicsRootSignature      = std::move(other.m_graphicsRootSignature);
		m_textureStorage             = std::move(other.m_textureStorage);
		m_textureManager             = std::move(other.m_textureManager);
		m_cameraManager              = std::move(other.m_cameraManager);
		m_dsvHeap                    = std::move(other.m_dsvHeap);
		m_backgroundColour           = other.m_backgroundColour;
		m_viewportAndScissors        = other.m_viewportAndScissors;
		m_temporaryDataBuffer        = std::move(other.m_temporaryDataBuffer);
		m_copyNecessary              = other.m_copyNecessary;

		return *this;
	}
};

template<
	typename MeshManager_t,
	typename GraphicsPipeline_t,
	typename Derived
>
class RenderEngineCommon : public RenderEngine
{
public:
	RenderEngineCommon(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	) : RenderEngine{ deviceManager, std::move(threadPool), frameCount },
		m_meshManager{ deviceManager.GetDevice(), &m_memoryManager },
		m_renderPassManager{ deviceManager.GetDevice() }
	{
		m_renderPassManager.SetDepthTesting(deviceManager.GetDevice(), &m_memoryManager, &m_dsvHeap);

		for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
			m_textureManager.SetDescriptorLayout(
				descriptorManager, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
			);
	}

	void SetShaderPath(const std::wstring& shaderPath) override
	{
		m_renderPassManager.SetShaderPath(shaderPath);
	}
	void SetSwapchainRTVFormat(DXGI_FORMAT rtvFormat) override
	{
		m_renderPassManager.SetRTVFormat(rtvFormat);
	}
	[[nodiscard]]
	std::uint32_t AddGraphicsPipeline(const ShaderName& pixelShader) override
	{
		return m_renderPassManager.AddOrGetGraphicsPipeline(pixelShader);
	}

	void MakePixelShaderRemovable(const ShaderName& pixelShader) noexcept override
	{
		m_renderPassManager.SetPSOOverwritable(pixelShader);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_meshManager.RemoveMeshBundle(bundleIndex);
	}

	void Resize(UINT width, UINT height) override
	{
		m_renderPassManager.ResizeDepthBuffer(width, height);

		m_viewportAndScissors.Resize(width, height);
	}

	[[nodiscard]]
	size_t GetCameraRegisterSlot() const noexcept override
	{
		return Derived::s_cameraCBVRegisterSlot;
	}

	void Render(size_t frameIndex, const RenderTarget& renderTarget) final
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
			frameIndex, renderTarget, counterValue, waitFence
		);
	}

protected:
	void Update(UINT64 frameIndex) const noexcept
	{
		m_cameraManager.Update(frameIndex);

		static_cast<Derived const*>(this)->_updatePerFrame(frameIndex);

		m_externalResourceManager.UpdateExtensionData(static_cast<size_t>(frameIndex));
	}

protected:
	MeshManager_t                            m_meshManager;
	D3DRenderPassManager<GraphicsPipeline_t> m_renderPassManager;

public:
	RenderEngineCommon(const RenderEngineCommon&) = delete;
	RenderEngineCommon& operator=(const RenderEngineCommon&) = delete;

	RenderEngineCommon(RenderEngineCommon&& other) noexcept
		: RenderEngine{ std::move(other) },
		m_meshManager{ std::move(other.m_meshManager) },
		m_renderPassManager{ std::move(other.m_renderPassManager) }
	{}
	RenderEngineCommon& operator=(RenderEngineCommon&& other) noexcept
	{
		RenderEngine::operator=(std::move(other));
		m_meshManager       = std::move(other.m_meshManager);
		m_renderPassManager = std::move(other.m_renderPassManager);

		return *this;
	}
};
#endif
