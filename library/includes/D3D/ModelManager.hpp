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
#include <D3DModelBuffer.hpp>
#include <MeshManager.hpp>
#include <PipelineManager.hpp>

template<
	class Derived,
	class ModelBundleType
>
class ModelManager
{
public:
	ModelManager(MemoryManager* memoryManager) : m_memoryManager{ memoryManager }, m_modelBundles{} {}

	void SetGraphicsConstantsRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept {
		static_cast<Derived*>(this)->_setGraphicsConstantRootIndex(
			descriptorManager, constantsRegisterSpace
		);
	}

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, std::uint32_t psoIndex,
		ModelBuffers& modelBuffers, StagingBufferManager& stagingManager,
		TemporaryDataBufferGPU& tempBuffer
	) {
		const std::vector<std::shared_ptr<Model>>& models = modelBundle->GetModels();

		auto bundleID = std::numeric_limits<std::uint32_t>::max();

		if (!std::empty(models))
		{
			std::vector<std::shared_ptr<Model>> copyModels = models;

			std::vector<std::uint32_t> modelIndices        = modelBuffers.AddMultipleRU32(
				std::move(copyModels)
			);

			ModelBundleType modelBundleObj{};

			static_cast<Derived*>(this)->ConfigureModelBundle(
				modelBundleObj, std::move(modelIndices), std::move(modelBundle),
				stagingManager, tempBuffer
			);

			modelBundleObj.SetPSOIndex(psoIndex);

			bundleID = modelBundleObj.GetID();

			AddModelBundle(std::move(modelBundleObj));
		}

		return bundleID;
	}

	void RemoveModelBundle(std::uint32_t bundleID, ModelBuffers& modelBuffers) noexcept
	{
		auto result = GetModelBundle(bundleID);

		if (result != std::end(m_modelBundles))
		{
			const auto modelBundleIndex = static_cast<size_t>(
				std::distance(std::begin(m_modelBundles), result)
			);

			static_cast<Derived*>(this)->ConfigureModelBundleRemove(modelBundleIndex, modelBuffers);

			m_modelBundles.erase(result);
		}
	}

	void ChangePSO(std::uint32_t bundleID, std::uint32_t psoIndex)
	{
		auto modelBundle = GetModelBundle(bundleID);

		if (modelBundle != std::end(m_modelBundles))
		{
			modelBundle->SetPSOIndex(psoIndex);

			ModelBundleType modelBundleObj = std::move(*modelBundle);

			m_modelBundles.erase(modelBundle);

			AddModelBundle(std::move(modelBundleObj));
		}
	}

protected:
	template<typename Pipeline>
	void BindPipeline(
		const ModelBundleType& modelBundle, const D3DCommandList& graphicsCmdList,
		const PipelineManager<Pipeline>& pipelineManager, size_t& previousPSOIndex
	) const noexcept {
		// PSO is more costly to bind, so the modelBundles are added in a way so they are sorted
		// by their PSO indices. And we only bind a new PSO, if the previous one was different.
		const size_t modelPSOIndex = modelBundle.GetPSOIndex();

		if (modelPSOIndex != previousPSOIndex)
		{
			pipelineManager.BindPipeline(modelPSOIndex, graphicsCmdList);

			previousPSOIndex = modelPSOIndex;
		}
	}

	[[nodiscard]]
	std::vector<ModelBundleType>::iterator GetModelBundle(std::uint32_t bundleID) noexcept
	{
		return std::ranges::find_if(
			m_modelBundles,
			[bundleID](const ModelBundleType& bundle) { return bundle.GetID() == bundleID; }
		);
	}

private:
	void AddModelBundle(ModelBundleType&& modelBundle) noexcept
	{
		const std::uint32_t psoIndex = modelBundle.GetPSOIndex();

		// These will be sorted by their PSO indices.
		// Upper_bound returns the next bigger element.
		auto result = std::ranges::upper_bound(
			m_modelBundles, psoIndex, {},
			[](const ModelBundleType& modelBundle)
			{
				return modelBundle.GetPSOIndex();
			}
		);

		// If the result is the end it, that means there is no bigger index. So, then
		// insert at the back. Otherwise, insert at the end of the range of the same indices.
		// Insert works for the end iterator, so no need to emplace_back.
		m_modelBundles.insert(result, std::move(modelBundle));
	}

protected:
	MemoryManager*               m_memoryManager;
	std::vector<ModelBundleType> m_modelBundles;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_memoryManager{ other.m_memoryManager },
		m_modelBundles{ std::move(other.m_modelBundles) }
	{}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		m_memoryManager = other.m_memoryManager;
		m_modelBundles  = std::move(other.m_modelBundles);

		return *this;
	}
};

class ModelManagerVSIndividual : public ModelManager<ModelManagerVSIndividual, ModelBundleVSIndividual>
{
	friend class ModelManager<ModelManagerVSIndividual, ModelBundleVSIndividual>;

	using Pipeline_t = GraphicsPipelineVSIndividualDraw;

public:
	ModelManagerVSIndividual(MemoryManager* memoryManager);

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
	);

	void Draw(
		const D3DCommandList& graphicsList, const MeshManagerVSIndividual& meshManager,
		const PipelineManager<Pipeline_t>& pipelineManager
	) const noexcept;

private:
	void _setGraphicsConstantRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;

	void ConfigureModelBundle(
		ModelBundleVSIndividual& modelBundleObj,
		std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle,
		StagingBufferManager& stagingManager,
		TemporaryDataBufferGPU& tempBuffer
	) const noexcept;

	void ConfigureModelBundleRemove(size_t bundleIndex, ModelBuffers& modelBuffers) noexcept;

private:
	UINT m_constantsRootIndex;

	// Vertex Shader ones
	static constexpr size_t s_constantDataCBVRegisterSlot = 0u;

public:
	ModelManagerVSIndividual(const ModelManagerVSIndividual&) = delete;
	ModelManagerVSIndividual& operator=(const ModelManagerVSIndividual&) = delete;

	ModelManagerVSIndividual(ModelManagerVSIndividual&& other) noexcept
		: ModelManager{ std::move(other) },
		m_constantsRootIndex{ other.m_constantsRootIndex }
	{}
	ModelManagerVSIndividual& operator=(ModelManagerVSIndividual&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_constantsRootIndex  = other.m_constantsRootIndex;

		return *this;
	}
};

class ModelManagerVSIndirect : public ModelManager<ModelManagerVSIndirect, ModelBundleVSIndirect>
{
	friend class ModelManager<ModelManagerVSIndirect, ModelBundleVSIndirect>;

	using GraphicsPipeline_t = GraphicsPipelineVSIndirectDraw;
	using ComputePipeline_t  = ComputePipeline;

	struct ConstantData
	{
		UINT modelCount;
	};

public:
	ModelManagerVSIndirect(
		ID3D12Device5* device, MemoryManager* memoryManager, std::uint32_t frameCount
	);

	void ResetCounterBuffer(const D3DCommandList& computeList, size_t frameIndex) const noexcept;

	void SetComputeConstantRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;

	void CopyOldBuffers(const D3DCommandList& copyList) noexcept;

	void SetDescriptorLayoutVS(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
	) const noexcept;

	void SetDescriptorLayoutCS(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
	) const noexcept;

	void SetDescriptors(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
	) const;

	void Draw(
		size_t frameIndex, const D3DCommandList& graphicsList, ID3D12CommandSignature* commandSignature,
		const MeshManagerVSIndirect& meshManager,
		const PipelineManager<GraphicsPipeline_t>& pipelineManager
	) const noexcept;
	void Dispatch(
		const D3DCommandList& computeList, const PipelineManager<ComputePipeline_t>& pipelineManager
	) const noexcept;

	[[nodiscard]]
	UINT GetConstantsVSRootIndex() const noexcept { return m_constantsVSRootIndex; }

	void UpdatePerFrame(UINT64 frameIndex, const MeshManagerVSIndirect& meshManager) const noexcept;

private:
	void _setGraphicsConstantRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;

	void ConfigureModelBundle(
		ModelBundleVSIndirect& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle, StagingBufferManager& stagingManager,
		TemporaryDataBufferGPU& tempBuffer
	);

	void ConfigureModelBundleRemove(size_t bundleIndex, ModelBuffers& modelBuffers) noexcept;

	void UpdateDispatchX() noexcept;
	void UpdateCounterResetValues();

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept
	{
		return static_cast<UINT>(sizeof(ConstantData) / sizeof(UINT));
	}

private:
	std::vector<SharedBufferCPU>          m_argumentInputBuffers;
	std::vector<SharedBufferGPUWriteOnly> m_argumentOutputBuffers;
	SharedBufferCPU                       m_cullingDataBuffer;
	std::vector<SharedBufferGPUWriteOnly> m_counterBuffers;
	Buffer                                m_counterResetBuffer;
	MultiInstanceCPUBuffer<std::uint32_t> m_meshBundleIndexBuffer;
	SharedBufferGPU                       m_perModelDataBuffer;
	UINT                                  m_dispatchXCount;
	UINT                                  m_argumentCount;
	UINT                                  m_constantsVSRootIndex;
	UINT                                  m_constantsCSRootIndex;

	// These CS models will have the data to be uploaded and the dispatching will be done on the Manager.
	std::vector<ModelBundleCSIndirect>    m_modelBundlesCS;
	bool                                  m_oldBufferCopyNecessary;

	// Vertex Shader ones
	// CBV
	static constexpr size_t s_constantDataVSCBVRegisterSlot = 0u;

	// Compute Shader ones
	// CBV
	static constexpr size_t s_constantDataCSCBVRegisterSlot      = 0u;
	// SRV
	static constexpr size_t s_argumentInputBufferSRVRegisterSlot = 1u;
	static constexpr size_t s_cullingDataBufferSRVRegisterSlot   = 2u;
	static constexpr size_t s_perModelDataSRVRegisterSlot        = 3u;
	static constexpr size_t s_meshBundleIndexSRVRegisterSlot     = 6u;
	// UAV
	static constexpr size_t s_argumenOutputUAVRegisterSlot       = 0u;
	static constexpr size_t s_counterUAVRegisterSlot             = 1u;

	// Each Compute Thread Group should have 64 threads.
	static constexpr float THREADBLOCKSIZE = 64.f;

public:
	ModelManagerVSIndirect(const ModelManagerVSIndirect&) = delete;
	ModelManagerVSIndirect& operator=(const ModelManagerVSIndirect&) = delete;

	ModelManagerVSIndirect(ModelManagerVSIndirect&& other) noexcept
		: ModelManager{ std::move(other) },
		m_argumentInputBuffers{ std::move(other.m_argumentInputBuffers) },
		m_argumentOutputBuffers{ std::move(other.m_argumentOutputBuffers) },
		m_cullingDataBuffer{ std::move(other.m_cullingDataBuffer) },
		m_counterBuffers{ std::move(other.m_counterBuffers) },
		m_counterResetBuffer{ std::move(other.m_counterResetBuffer) },
		m_meshBundleIndexBuffer{ std::move(other.m_meshBundleIndexBuffer) },
		m_perModelDataBuffer{ std::move(other.m_perModelDataBuffer) },
		m_dispatchXCount{ other.m_dispatchXCount },
		m_argumentCount{ other.m_argumentCount },
		m_constantsVSRootIndex{ other.m_constantsVSRootIndex },
		m_constantsCSRootIndex{ other.m_constantsCSRootIndex },
		m_modelBundlesCS{ std::move(other.m_modelBundlesCS) },
		m_oldBufferCopyNecessary{ other.m_oldBufferCopyNecessary }
	{}
	ModelManagerVSIndirect& operator=(ModelManagerVSIndirect&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_argumentInputBuffers   = std::move(other.m_argumentInputBuffers);
		m_argumentOutputBuffers  = std::move(other.m_argumentOutputBuffers);
		m_cullingDataBuffer      = std::move(other.m_cullingDataBuffer);
		m_counterBuffers         = std::move(other.m_counterBuffers);
		m_counterResetBuffer     = std::move(other.m_counterResetBuffer);
		m_meshBundleIndexBuffer  = std::move(other.m_meshBundleIndexBuffer);
		m_perModelDataBuffer     = std::move(other.m_perModelDataBuffer);
		m_dispatchXCount         = other.m_dispatchXCount;
		m_argumentCount          = other.m_argumentCount;
		m_constantsVSRootIndex   = other.m_constantsVSRootIndex;
		m_constantsCSRootIndex   = other.m_constantsCSRootIndex;
		m_modelBundlesCS         = std::move(other.m_modelBundlesCS);
		m_oldBufferCopyNecessary = other.m_oldBufferCopyNecessary;

		return *this;
	}
};

class ModelManagerMS : public ModelManager<ModelManagerMS, ModelBundleMSIndividual>
{
	friend class ModelManager<ModelManagerMS, ModelBundleMSIndividual>;

	using Pipeline_t = GraphicsPipelineMS;

public:
	ModelManagerMS(MemoryManager* memoryManager);

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
	) const noexcept;

	void Draw(
		const D3DCommandList& graphicsList, const MeshManagerMS& meshManager,
		const PipelineManager<Pipeline_t>& pipelineManager
	) const noexcept;

private:
	void _setGraphicsConstantRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;
	void ConfigureModelBundle(
		ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle, StagingBufferManager& stagingManager,
		TemporaryDataBufferGPU& tempBuffer
	);

	void ConfigureModelBundleRemove(size_t bundleIndex, ModelBuffers& modelBuffers) noexcept;

	void _updatePerFrame([[maybe_unused]] UINT64 frameIndex) const noexcept {}

private:
	UINT m_constantsRootIndex;

	// CBV
	static constexpr size_t s_constantDataCBVRegisterSlot = 0u;

public:
	ModelManagerMS(const ModelManagerMS&) = delete;
	ModelManagerMS& operator=(const ModelManagerMS&) = delete;

	ModelManagerMS(ModelManagerMS&& other) noexcept
		: ModelManager{ std::move(other) },
		m_constantsRootIndex{ other.m_constantsRootIndex }
	{}
	ModelManagerMS& operator=(ModelManagerMS&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_constantsRootIndex  = other.m_constantsRootIndex;

		return *this;
	}
};
#endif
