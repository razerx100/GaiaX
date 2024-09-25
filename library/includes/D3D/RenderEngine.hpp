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
#include <CommonBuffers.hpp>
#include <TextureManager.hpp>
#include <DepthBuffer.hpp>
#include <CameraManager.hpp>
#include <ViewportAndScissorManager.hpp>
#include <D3DRootSignature.hpp>
#include <D3DRenderTarget.hpp>
#include <Model.hpp>
#include <Shader.hpp>
#include <MeshBundle.hpp>

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

	[[nodiscard]]
	size_t AddMaterial(std::shared_ptr<Material> material);
	[[nodiscard]]
	std::vector<size_t> AddMaterials(std::vector<std::shared_ptr<Material>>&& materials);

	void UpdateMaterial(size_t index) const noexcept { m_materialBuffers.Update(index); }
	void RemoveMaterial(size_t index) noexcept { m_materialBuffers.Remove(index); }

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

	virtual void SetShaderPath(const std::wstring& shaderPath) = 0;
	virtual void AddPixelShader(const ShaderName& pixelShader) = 0;
	virtual void ChangePixelShader(
		std::uint32_t modelBundleID, const ShaderName& pixelShader
	) = 0;

	[[nodiscard]]
	virtual void Render(size_t frameIndex, const RenderTarget& renderTarget) = 0;
	virtual void Resize(UINT width, UINT height) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& pixelShader
	);
	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundleMS>&& modelBundle, const ShaderName& pixelShader
	);

	virtual void RemoveModelBundle(std::uint32_t bundleID) noexcept = 0;

	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle);
	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle);

	virtual void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept = 0;

	// Making this function, so this can be overriden to add the compute queues in
	// certain Engines.
	virtual void WaitForGPUToFinish();

	[[nodiscard]]
	const DepthBuffer& GetDepthBuffer() const noexcept { return m_depthBuffer; }
	[[nodiscard]]
	const D3DCommandQueue& GetPresentQueue() const noexcept { return m_graphicsQueue; }

protected:
	[[nodiscard]]
	virtual size_t GetCameraRegisterSlot() const noexcept = 0;

	void SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY cameraShaderVisibility) noexcept;

	virtual void Update(UINT64 frameIndex) const noexcept;

protected:
	// These descriptors are bound to the pixel shader. So, they should be the same across
	// all of the pipeline types. That's why we are going to bind them to their own RegisterSpace.
	static constexpr size_t s_graphicsPipelineSetLayoutCount = 2u;
	static constexpr size_t s_vertexShaderRegisterSpace      = 0u;
	static constexpr size_t s_pixelShaderRegisterSpace       = 1u;

	static constexpr size_t s_materialSRVRegisterSlot = 1u;
	static constexpr size_t s_textureSRVRegisterSlot  = 2u;

protected:
	std::shared_ptr<ThreadPool>       m_threadPool;
	MemoryManager                     m_memoryManager;
	std::vector<UINT64>               m_counterValues;
	D3DCommandQueue                   m_graphicsQueue;
	std::vector<D3DFence>             m_graphicsWait;
	D3DCommandQueue                   m_copyQueue;
	std::vector<D3DFence>             m_copyWait;
	StagingBufferManager              m_stagingManager;
	std::vector<D3DDescriptorManager> m_graphicsDescriptorManagers;
	D3DRootSignature                  m_graphicsRootSignature;
	TextureStorage                    m_textureStorage;
	TextureManager                    m_textureManager;
	MaterialBuffers                   m_materialBuffers;
	CameraManager                     m_cameraManager;
	D3DReusableDescriptorHeap         m_dsvHeap;
	DepthBuffer                       m_depthBuffer;
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
		m_graphicsDescriptorManagers{ std::move(other.m_graphicsDescriptorManagers) },
		m_graphicsRootSignature{ std::move(other.m_graphicsRootSignature) },
		m_textureStorage{ std::move(other.m_textureStorage) },
		m_textureManager{ std::move(other.m_textureManager) },
		m_materialBuffers{ std::move(other.m_materialBuffers) },
		m_cameraManager{ std::move(other.m_cameraManager) },
		m_dsvHeap{ std::move(other.m_dsvHeap) },
		m_depthBuffer{ std::move(other.m_depthBuffer) },
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
		m_graphicsDescriptorManagers = std::move(other.m_graphicsDescriptorManagers);
		m_graphicsRootSignature      = std::move(other.m_graphicsRootSignature);
		m_textureStorage             = std::move(other.m_textureStorage);
		m_textureManager             = std::move(other.m_textureManager);
		m_materialBuffers            = std::move(other.m_materialBuffers);
		m_cameraManager              = std::move(other.m_cameraManager);
		m_dsvHeap                    = std::move(other.m_dsvHeap);
		m_depthBuffer                = std::move(other.m_depthBuffer);
		m_backgroundColour           = other.m_backgroundColour;
		m_viewportAndScissors        = other.m_viewportAndScissors;
		m_temporaryDataBuffer        = std::move(other.m_temporaryDataBuffer);
		m_copyNecessary              = other.m_copyNecessary;

		return *this;
	}
};

template<typename ModelManager_t, typename Derived>
class RenderEngineCommon : public RenderEngine
{
public:
	RenderEngineCommon(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	) : RenderEngine{ deviceManager, std::move(threadPool), frameCount },
		m_modelManager{
			Derived::GetModelManager(
				deviceManager, &m_memoryManager, &m_stagingManager,
				static_cast<std::uint32_t>(frameCount)
			)
		}, m_pipelineStages{}
	{
		for (auto& descriptorManager : m_graphicsDescriptorManagers)
			m_textureManager.SetDescriptorLayout(
				descriptorManager, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
			);
	}

	void SetShaderPath(const std::wstring& shaderPath) override
	{
		m_modelManager.SetShaderPath(shaderPath);
	}
	void AddPixelShader(const ShaderName& pixelShader) override
	{
		m_modelManager.AddPSO(pixelShader);
	}
	void ChangePixelShader(std::uint32_t modelBundleID, const ShaderName& pixelShader) override
	{
		m_modelManager.ChangePSO(modelBundleID, pixelShader);
	}

	void RemoveModelBundle(std::uint32_t bundleID) noexcept override
	{
		m_modelManager.RemoveModelBundle(bundleID);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_modelManager.RemoveMeshBundle(bundleIndex);
	}

	void Resize(UINT width, UINT height) override
	{
		m_depthBuffer.Create(width, height);

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

		for (auto pipelineStage : m_pipelineStages)
			waitFence = (static_cast<Derived*>(this)->*pipelineStage)(
				frameIndex, renderTarget, counterValue, waitFence
			);
	}

protected:
	void Update(UINT64 frameIndex) const noexcept override
	{
		RenderEngine::Update(frameIndex);

		m_modelManager.UpdatePerFrame(frameIndex);
	}

	using PipelineSignature = ID3D12Fence*(Derived::*)(
			size_t, const RenderTarget&, UINT64&, ID3D12Fence*
		);

protected:
	ModelManager_t                 m_modelManager;
	std::vector<PipelineSignature> m_pipelineStages;

public:
	RenderEngineCommon(const RenderEngineCommon&) = delete;
	RenderEngineCommon& operator=(const RenderEngineCommon&) = delete;

	RenderEngineCommon(RenderEngineCommon&& other) noexcept
		: RenderEngine{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) },
		m_pipelineStages{ std::move(other.m_pipelineStages) }
	{}
	RenderEngineCommon& operator=(RenderEngineCommon&& other) noexcept
	{
		RenderEngine::operator=(std::move(other));
		m_modelManager        = std::move(other.m_modelManager);
		m_pipelineStages      = std::move(other.m_pipelineStages);

		return *this;
	}
};
#endif
