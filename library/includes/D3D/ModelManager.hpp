#ifndef MODEL_MANAGER_HPP_
#define MODEL_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <utility>
#include <vector>
#include <ranges>
#include <algorithm>
#include <Model.hpp>
#include <D3DCommandQueue.hpp>
#include <ReusableD3DBuffer.hpp>
#include <GraphicsPipelineVS.hpp>
#include <GraphicsPipelineMS.hpp>
#include <ComputePipeline.hpp>
#include <D3DModelBundle.hpp>
#include <MeshManager.hpp>
#include <PipelineManager.hpp>

template<class ModelBundleType>
class ModelManager
{
public:
	ModelManager() : m_modelBundles{} {}

	[[nodiscard]]
	std::uint32_t AddPipelineToModelBundle(
		std::uint32_t bundleIndex, std::uint32_t pipelineIndex
	) noexcept {
		return m_modelBundles[bundleIndex].AddPipeline(pipelineIndex);
	}

	[[nodiscard]]
	std::optional<size_t> GetPipelineLocalIndex(
		std::uint32_t bundleIndex, std::uint32_t pipelineIndex
	) const noexcept {
		return m_modelBundles[bundleIndex].GetPipelineLocalIndex(pipelineIndex);
	}

protected:
	ReusableVector<ModelBundleType> m_modelBundles;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_modelBundles{ std::move(other.m_modelBundles) }
	{}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		m_modelBundles = std::move(other.m_modelBundles);

		return *this;
	}
};

template<class ModelBundleType>
class ModelManagerCommon : public ModelManager<ModelBundleType>
{
	friend class ModelManager<ModelBundleType>;

public:
	ModelManagerCommon() : ModelManager<ModelBundleType>{} {}

	void ChangeModelPipeline(
		std::uint32_t bundleIndex, std::uint32_t modelIndexInBundle, std::uint32_t oldPipelineIndex,
		std::uint32_t newPipelineIndex
	) {
		this->m_modelBundles[bundleIndex].MoveModel(
			modelIndexInBundle, oldPipelineIndex, newPipelineIndex
		);
	}

	// Will think about Adding a new model later.
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, std::vector<std::uint32_t>&& modelBufferIndices
	) {
		const size_t bundleIndex          = this->m_modelBundles.Add(ModelBundleType{});

		ModelBundleType& localModelBundle = this->m_modelBundles[bundleIndex];

		localModelBundle.SetModelIndices(std::move(modelBufferIndices));

		_addModelsFromBundle(localModelBundle, *modelBundle);

		localModelBundle.SetModelBundle(std::move(modelBundle));

		return static_cast<std::uint32_t>(bundleIndex);
	}

	// Returns the stored model indices of the Model Buffers
	[[nodiscard]]
	std::vector<std::uint32_t> RemoveModelBundle(std::uint32_t bundleIndex) noexcept
	{
		const size_t bundleIndexST        = bundleIndex;

		ModelBundleType& localModelBundle = this->m_modelBundles[bundleIndexST];

		std::vector<std::uint32_t> modelBufferIndices = localModelBundle.TakeModelBufferIndices();

		localModelBundle.CleanupData();

		this->m_modelBundles.RemoveElement(bundleIndexST);

		return modelBufferIndices;
	}

private:
	void _addModelsFromBundle(
		ModelBundleType& localModelBundle, const ModelBundle& modelBundle
	) {
		const std::vector<std::shared_ptr<Model>>& models = modelBundle.GetModels();

		const size_t modelCount = std::size(models);

		for (size_t index = 0u; index < modelCount; ++index)
		{
			const std::shared_ptr<Model>& model = models[index];

			localModelBundle.AddModel(model->GetPipelineIndex(), static_cast<std::uint32_t>(index));
		}
	}

public:
	ModelManagerCommon(const ModelManagerCommon&) = delete;
	ModelManagerCommon& operator=(const ModelManagerCommon&) = delete;

	ModelManagerCommon(ModelManagerCommon&& other) noexcept
		: ModelManager<ModelBundleType>{ std::move(other) }
	{}
	ModelManagerCommon& operator=(ModelManagerCommon&& other) noexcept
	{
		ModelManager<ModelBundleType>::operator=(std::move(other));

		return *this;
	}
};

class ModelManagerVSIndividual : public ModelManagerCommon<ModelBundleVSIndividual>
{
	using Pipeline_t = GraphicsPipelineVSIndividualDraw;

public:
	ModelManagerVSIndividual() : m_constantsRootIndex{ 0u }, ModelManagerCommon {} {}

	void SetGraphicsConstantsRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
	);

	void DrawPipeline(
		size_t modelBundleIndex, size_t pipelineLocalIndex, const D3DCommandList& graphicsList,
		const MeshManagerVSIndividual& meshManager
	) const noexcept;

private:
	UINT m_constantsRootIndex;

	// Vertex Shader ones
	static constexpr size_t s_constantDataCBVRegisterSlot = 0u;

public:
	ModelManagerVSIndividual(const ModelManagerVSIndividual&) = delete;
	ModelManagerVSIndividual& operator=(const ModelManagerVSIndividual&) = delete;

	ModelManagerVSIndividual(ModelManagerVSIndividual&& other) noexcept
		: ModelManagerCommon{ std::move(other) },
		m_constantsRootIndex{ other.m_constantsRootIndex }
	{}
	ModelManagerVSIndividual& operator=(ModelManagerVSIndividual&& other) noexcept
	{
		ModelManagerCommon::operator=(std::move(other));
		m_constantsRootIndex        = other.m_constantsRootIndex;

		return *this;
	}
};

class ModelManagerVSIndirect : public ModelManager<ModelBundleVSIndirect>
{
	using GraphicsPipeline_t = GraphicsPipelineVSIndirectDraw;
	using ComputePipeline_t  = ComputePipeline;

	struct ConstantData
	{
		UINT allocatedModelCount;
	};

	struct PerModelBundleData
	{
		std::uint32_t meshBundleIndex;
	};

public:
	ModelManagerVSIndirect(
		ID3D12Device5* device, MemoryManager* memoryManager, std::uint32_t frameCount
	);

	void ResetCounterBuffer(const D3DCommandList& computeList, size_t frameIndex) const noexcept;

	void SetCSPSOIndex(std::uint32_t psoIndex) noexcept { m_csPSOIndex = psoIndex; }

	void SetComputeConstantsRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;
	void SetGraphicsConstantsRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;

	// Will think about Adding a new model later.
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, std::vector<std::uint32_t>&& modelBufferIndices
	);

	// Returns the stored model indices of the Model Buffers
	[[nodiscard]]
	std::vector<std::uint32_t> RemoveModelBundle(std::uint32_t bundleIndex) noexcept;

	void SetDescriptorLayoutVS(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
	) const noexcept;

	void SetDescriptorLayoutCS(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
	) const noexcept;

	void SetDescriptors(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
	) const;

	void ChangeModelPipeline(
		std::uint32_t bundleIndex, std::uint32_t modelIndexInBundle, std::uint32_t oldPipelineIndex,
		std::uint32_t newPipelineIndex
	);

	void DrawPipeline(
		size_t frameIndex, size_t modelBundleIndex, size_t pipelineLocalIndex,
		const D3DCommandList& graphicsList, ID3D12CommandSignature* commandSignature,
		const MeshManagerVSIndirect& meshManager
	) const noexcept;

	void Dispatch(
		const D3DCommandList& computeList, const PipelineManager<ComputePipeline_t>& pipelineManager
	) const noexcept;

	[[nodiscard]]
	UINT GetConstantsVSRootIndex() const noexcept { return m_constantsVSRootIndex; }

	void UpdatePipelinePerFrame(
		UINT64 frameIndex, size_t modelBundleIndex, size_t pipelineLocalIndex,
		const MeshManagerVSIndirect& meshManager, bool skipCulling
	) const noexcept;

private:
	void _addModelsFromBundle(
		ModelBundleVSIndirect& localModelBundle, const ModelBundle& modelBundle,
		std::uint32_t modelBundleIndex
	);

	void UpdateAllocatedModelCount() noexcept;
	void UpdateCounterResetValues();

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept
	{
		return static_cast<UINT>(sizeof(ConstantData) / sizeof(UINT));
	}

private:
	std::vector<SharedBufferCPU>               m_argumentInputBuffers;
	std::vector<SharedBufferGPUWriteOnly>      m_argumentOutputBuffers;
	SharedBufferCPU                            m_perPipelineBuffer;
	std::vector<SharedBufferGPUWriteOnly>      m_counterBuffers;
	Buffer                                     m_counterResetBuffer;
	MultiInstanceCPUBuffer<PerModelBundleData> m_perModelBundleBuffer;
	SharedBufferCPU                            m_perModelBuffer;
	UINT                                       m_dispatchXCount;
	UINT                                       m_allocatedModelCount;
	std::uint32_t                              m_csPSOIndex;
	UINT                                       m_constantsVSRootIndex;
	UINT                                       m_constantsCSRootIndex;

	// Vertex Shader ones
	// CBV
	static constexpr size_t s_constantDataVSCBVRegisterSlot = 0u;

	// Compute Shader ones
	// CBV
	static constexpr size_t s_constantDataCSCBVRegisterSlot = 0u;
	// SRV
	static constexpr size_t s_argumentInputSRVRegisterSlot  = 1u;
	static constexpr size_t s_perPipelineSRVRegisterSlot    = 2u;
	static constexpr size_t s_perModelSRVRegisterSlot       = 3u;
	static constexpr size_t s_perModelBundleSRVRegisterSlot = 6u;
	// UAV
	static constexpr size_t s_argumenOutputUAVRegisterSlot  = 0u;
	static constexpr size_t s_counterUAVRegisterSlot        = 1u;

	// Each Compute Thread Group should have 64 threads.
	static constexpr float THREADBLOCKSIZE = 64.f;

public:
	ModelManagerVSIndirect(const ModelManagerVSIndirect&) = delete;
	ModelManagerVSIndirect& operator=(const ModelManagerVSIndirect&) = delete;

	ModelManagerVSIndirect(ModelManagerVSIndirect&& other) noexcept
		: ModelManager{ std::move(other) },
		m_argumentInputBuffers{ std::move(other.m_argumentInputBuffers) },
		m_argumentOutputBuffers{ std::move(other.m_argumentOutputBuffers) },
		m_perPipelineBuffer{ std::move(other.m_perPipelineBuffer) },
		m_counterBuffers{ std::move(other.m_counterBuffers) },
		m_counterResetBuffer{ std::move(other.m_counterResetBuffer) },
		m_perModelBundleBuffer{ std::move(other.m_perModelBundleBuffer) },
		m_perModelBuffer{ std::move(other.m_perModelBuffer) },
		m_dispatchXCount{ other.m_dispatchXCount },
		m_allocatedModelCount{ other.m_allocatedModelCount },
		m_csPSOIndex{ other.m_csPSOIndex },
		m_constantsVSRootIndex{ other.m_constantsVSRootIndex },
		m_constantsCSRootIndex{ other.m_constantsCSRootIndex }
	{}
	ModelManagerVSIndirect& operator=(ModelManagerVSIndirect&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_argumentInputBuffers   = std::move(other.m_argumentInputBuffers);
		m_argumentOutputBuffers  = std::move(other.m_argumentOutputBuffers);
		m_perPipelineBuffer      = std::move(other.m_perPipelineBuffer);
		m_counterBuffers         = std::move(other.m_counterBuffers);
		m_counterResetBuffer     = std::move(other.m_counterResetBuffer);
		m_perModelBundleBuffer   = std::move(other.m_perModelBundleBuffer);
		m_perModelBuffer         = std::move(other.m_perModelBuffer);
		m_dispatchXCount         = other.m_dispatchXCount;
		m_allocatedModelCount    = other.m_allocatedModelCount;
		m_csPSOIndex             = other.m_csPSOIndex;
		m_constantsVSRootIndex   = other.m_constantsVSRootIndex;
		m_constantsCSRootIndex   = other.m_constantsCSRootIndex;

		return *this;
	}
};

class ModelManagerMS : public ModelManagerCommon<ModelBundleMSIndividual>
{
	using Pipeline_t = GraphicsPipelineMS;

public:
	ModelManagerMS() : ModelManagerCommon{}, m_constantsRootIndex{ 0u } {}

	void SetGraphicsConstantsRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
	) const noexcept;

	void DrawPipeline(
		size_t modelBundleIndex, size_t pipelineLocalIndex, const D3DCommandList& graphicsList,
		const MeshManagerMS& meshManager
	) const noexcept;

private:
	UINT m_constantsRootIndex;

	// CBV
	static constexpr size_t s_constantDataCBVRegisterSlot = 0u;

public:
	ModelManagerMS(const ModelManagerMS&) = delete;
	ModelManagerMS& operator=(const ModelManagerMS&) = delete;

	ModelManagerMS(ModelManagerMS&& other) noexcept
		: ModelManagerCommon{ std::move(other) },
		m_constantsRootIndex{ other.m_constantsRootIndex }
	{}
	ModelManagerMS& operator=(ModelManagerMS&& other) noexcept
	{
		ModelManagerCommon::operator=(std::move(other));
		m_constantsRootIndex        = other.m_constantsRootIndex;

		return *this;
	}
};
#endif
