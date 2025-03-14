#ifndef D3D_MODEL_BUNDLE_HPP_
#define D3D_MODEL_BUNDLE_HPP_
#include <memory>
#include <vector>
#include <unordered_map>
#include <Model.hpp>
#include <ReusableVector.hpp>
#include <D3DMeshBundleMS.hpp>
#include <D3DMeshBundleVS.hpp>
#include <GraphicsPipelineVS.hpp>
#include <GraphicsPipelineMS.hpp>
#include <PipelineManager.hpp>

class PipelineModelsBase
{
public:
	struct ModelData
	{
		std::uint32_t bundleIndex;
		std::uint32_t bufferIndex;

		bool operator==(const ModelData& other) const noexcept
		{
			return bundleIndex == other.bundleIndex && bufferIndex == other.bufferIndex;
		}
	};

protected:
	using ModelDataIt = std::vector<ModelData>::const_iterator;

	struct SortedData
	{
		ModelDataIt it;
		float       worldZ;
	};

public:
	PipelineModelsBase() : m_psoIndex{ 0u } {}

	void SetPSOIndex(std::uint32_t index) noexcept { m_psoIndex = index; }

	[[nodiscard]]
	std::uint32_t GetPSOIndex() const noexcept { return m_psoIndex; }

	[[nodiscard]]
	static D3D12_DRAW_INDEXED_ARGUMENTS GetDrawIndexedIndirectCommand(
		const MeshTemporaryDetailsVS& meshDetailsVS
	) noexcept;

	[[nodiscard]]
	std::uint32_t AddModel(std::uint32_t bundleIndex, std::uint32_t bufferIndex) noexcept;
	[[nodiscard]]
	// This won't actually sort it. It will be sorted in the DrawSorted function.
	std::uint32_t AddModelSorted(std::uint32_t bundleIndex, std::uint32_t bufferIndex) noexcept;
	[[nodiscard]]
	const ModelData& GetModelData(size_t localIndex) const noexcept
	{
		return m_modelData[localIndex];
	}

	void RemoveModel(std::uint32_t localIndex) noexcept
	{
		m_modelData.RemoveElement(localIndex);
	}

	void RemoveModelSorted(std::uint32_t localIndex) noexcept;

protected:
	[[nodiscard]]
	static float GetWorldZ(const AxisAlignedBoundingBox& aabb, const DirectX::XMMATRIX& world) noexcept;

	template<class MeshBundle_t>
	void SortDetails(
		const MeshBundle_t& meshBundle, const std::vector<std::shared_ptr<Model>>& models
	) noexcept {
		const size_t modelCount = std::size(m_sortedData);

		for (size_t index = 0u; index < modelCount; ++index)
		{
			const ModelData& modelData          = *m_sortedData[index].it;
			const std::shared_ptr<Model>& model = models[modelData.bundleIndex];

			if (!model->IsVisible())
				continue;

			const auto& meshDetails = meshBundle.GetMeshDetails(model->GetMeshIndex());

			m_sortedData[index].worldZ
				= GetWorldZ(meshDetails.aabb, model->GetModelMatrix()) + model->GetModelOffset().z;
		}

		// Sort
		std::ranges::sort(
			m_sortedData,
			[](const SortedData& element1, const SortedData& element2) -> bool
			{
				return element1.worldZ < element2.worldZ;
			}
		);
	}

	template<bool Sorted>
	[[nodiscard]]
	const ModelData& _getModelData(size_t index) const noexcept
	{
		if constexpr (Sorted)
			return *m_sortedData[index].it;
		else
			return m_modelData[index];
	}
	template<bool Sorted>
	[[nodiscard]]
	bool _isModelInUse(size_t index) const noexcept
	{
		// In sorted mode, the iterators of the actual models will be sorted and
		// the iterators of the any inactive iterators will be removed. So, it
		// will always be in use.
		if constexpr (Sorted)
			return true;
		else
			return m_modelData.IsInUse(index);
	}

	void _cleanupData() noexcept { operator=(PipelineModelsBase{}); }

protected:
	std::uint32_t             m_psoIndex;
	ReusableVector<ModelData> m_modelData;
	std::vector<SortedData>   m_sortedData;

public:
	PipelineModelsBase(const PipelineModelsBase&) = delete;
	PipelineModelsBase& operator=(const PipelineModelsBase&) = delete;

	PipelineModelsBase(PipelineModelsBase&& other) noexcept
		: m_psoIndex{ other.m_psoIndex },
		m_modelData{ std::move(other.m_modelData) },
		m_sortedData{ std::move(other.m_sortedData) }
	{}
	PipelineModelsBase& operator=(PipelineModelsBase&& other) noexcept
	{
		m_psoIndex   = other.m_psoIndex;
		m_modelData  = std::move(other.m_modelData);
		m_sortedData = std::move(other.m_sortedData);

		return *this;
	}
};

class PipelineModelsVSIndividual: public PipelineModelsBase
{
public:
	PipelineModelsVSIndividual() : PipelineModelsBase{} {}

	void CleanupData() noexcept { _cleanupData(); }

	void Draw(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

	void DrawSorted(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) noexcept;

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept { return 1u; }

private:
	template<bool Sorted>
	void _draw(
		size_t modelCount, const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleVS& meshBundle, const std::vector<std::shared_ptr<Model>>& models
	) const noexcept {
		ID3D12GraphicsCommandList* cmdList = graphicsList.Get();

		for (size_t index = 0; index < modelCount; ++index)
			DrawModel(
				_isModelInUse<Sorted>(index), _getModelData<Sorted>(index), cmdList, constantsRootIndex,
				meshBundle, models
			);
	}

	void DrawModel(
		bool isInUse, const ModelData& modelData, ID3D12GraphicsCommandList* graphicsList,
		UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

public:
	PipelineModelsVSIndividual(const PipelineModelsVSIndividual&) = delete;
	PipelineModelsVSIndividual& operator=(const PipelineModelsVSIndividual&) = delete;

	PipelineModelsVSIndividual(PipelineModelsVSIndividual&& other) noexcept
		: PipelineModelsBase{ std::move(other) }
	{}
	PipelineModelsVSIndividual& operator=(PipelineModelsVSIndividual&& other) noexcept
	{
		PipelineModelsBase::operator=(std::move(other));

		return *this;
	}
};

class PipelineModelsMSIndividual : public PipelineModelsBase
{
public:
	// In D3D12, root constants/CBV are laid out as vec4, so we need
	// to either align to 16bytes or pass this data at the end and gonna do that.
	struct MeshDetails
	{
		std::uint32_t meshletCount;
		std::uint32_t meshletOffset;
		std::uint32_t indexOffset;
		std::uint32_t primOffset;
		std::uint32_t vertexOffset;
	};
	struct ModelDetailsMS
	{
		MeshDetails   meshDetails;
		std::uint32_t modelBufferIndex;
	};

public:
	PipelineModelsMSIndividual() : PipelineModelsBase{} {}

	void CleanupData() noexcept { _cleanupData(); }

	void Draw(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

	void DrawSorted(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) noexcept;

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept
	{
		return static_cast<UINT>(sizeof(ModelDetailsMS) / sizeof(UINT));
	}

private:
	[[nodiscard]]
	static UINT DivRoundUp(UINT num, UINT den) noexcept
	{
		return (num + den - 1) / den;
	}

	template<bool Sorted>
	void _draw(
		size_t modelCount, const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleMS& meshBundle, const std::vector<std::shared_ptr<Model>>& models
	) const noexcept {
		ID3D12GraphicsCommandList6* cmdList = graphicsList.Get();

		for (size_t index = 0u; index < modelCount; ++index)
			DrawModel(
				_isModelInUse<Sorted>(index), _getModelData<Sorted>(index), cmdList, constantsRootIndex,
				meshBundle, models
			);
	}

	void DrawModel(
		bool isInUse, const ModelData& modelData, ID3D12GraphicsCommandList6* graphicsList,
		UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

private:
	static constexpr UINT s_amplificationLaneCount = 32u;

public:
	PipelineModelsMSIndividual(const PipelineModelsMSIndividual&) = delete;
	PipelineModelsMSIndividual& operator=(const PipelineModelsMSIndividual&) = delete;

	PipelineModelsMSIndividual(PipelineModelsMSIndividual&& other) noexcept
		: PipelineModelsBase{ std::move(other) }
	{}
	PipelineModelsMSIndividual& operator=(PipelineModelsMSIndividual&& other) noexcept
	{
		PipelineModelsBase::operator=(std::move(other));

		return *this;
	}
};

class PipelineModelsCSIndirect : public PipelineModelsBase
{
public:
	struct PerPipelineData
	{
		std::uint32_t modelCount;
		std::uint32_t modelOffset;
		std::uint32_t modelBundleIndex;
	};

	struct IndirectArgument
	{
		UINT                         modelIndex;
		D3D12_DRAW_INDEXED_ARGUMENTS drawArguments;
	};

	struct PerModelData
	{
		std::uint32_t pipelineIndex;
		std::uint32_t isVisible;
	};

	struct IndexLink
	{
		std::uint32_t bundleIndex;
		std::uint32_t localIndex;
	};

public:
	PipelineModelsCSIndirect();

	void CleanupData() noexcept { operator=(PipelineModelsCSIndirect{}); }

	[[nodiscard]]
	size_t GetAddableModelCount() const noexcept
	{
		return m_modelData.GetIndicesManager().GetFreeIndexCount();
	}

	[[nodiscard]]
	std::vector<IndexLink> RemoveInactiveModels() noexcept;

	void UpdateNonPerFrameData(std::uint32_t modelBundleIndex, bool areSortedModels) noexcept;

	void AllocateBuffers(
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer,
		bool areSortedModels
	);

	void ResetCullingData() const noexcept;

	void Update(
		size_t frameIndex, const D3DMeshBundleVS& meshBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;
	void UpdateSorted(
		size_t frameIndex, const D3DMeshBundleVS& meshBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) noexcept;

	void RelinquishMemory(
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer
	) noexcept;

	[[nodiscard]]
	size_t GetModelCount(bool areSortedModels) const noexcept
	{
		return areSortedModels ? std::size(m_sortedData) : std::size(m_modelData);
	}

	[[nodiscard]]
	static consteval size_t GetPerModelStride() noexcept
	{
		return sizeof(PerModelData);
	}

private:
	template<bool Sorted>
	void _update(
		size_t modelCount, size_t frameIndex, const D3DMeshBundleVS& meshBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept {
		if (!modelCount)
			return;

		const SharedBufferData& argumentInputSharedData = m_argumentInputSharedData[frameIndex];

		std::uint8_t* argumentInputStart = argumentInputSharedData.bufferData->CPUHandle();
		std::uint8_t* perModelStart      = m_perModelSharedData.bufferData->CPUHandle();

		constexpr size_t argumentStride = sizeof(IndirectArgument);
		auto argumentOffset             = static_cast<size_t>(argumentInputSharedData.offset);

		constexpr size_t perModelStride = sizeof(PerModelData);
		auto perModelOffset             = static_cast<size_t>(m_perModelSharedData.offset);
		constexpr auto isVisibleOffset  = offsetof(PerModelData, isVisible);

		for (size_t index = 0u; index < modelCount; ++index)
		{
			const ModelData& modelData          = _getModelData<Sorted>(index);
			const std::shared_ptr<Model>& model = models[modelData.bundleIndex];

			const MeshTemporaryDetailsVS& meshDetailsVS = meshBundle.GetMeshDetails(model->GetMeshIndex());
			const D3D12_DRAW_INDEXED_ARGUMENTS meshArgs = PipelineModelsBase::GetDrawIndexedIndirectCommand(
				meshDetailsVS
			);

			IndirectArgument arguments
			{
				.modelIndex    = modelData.bufferIndex,
				.drawArguments = meshArgs
			};

			memcpy(argumentInputStart + argumentOffset, &arguments, argumentStride);

			argumentOffset += argumentStride;

			// Model Visiblity
			const auto visiblity = static_cast<std::uint32_t>(
				_isModelInUse<Sorted>(index) && model->IsVisible()
			);

			memcpy(perModelStart + perModelOffset + isVisibleOffset, &visiblity, sizeof(std::uint32_t));

			perModelOffset += perModelStride;
		}
	}

private:
	SharedBufferData              m_perPipelineSharedData;
	SharedBufferData              m_perModelSharedData;
	std::vector<SharedBufferData> m_argumentInputSharedData;

public:
	PipelineModelsCSIndirect(const PipelineModelsCSIndirect&) = delete;
	PipelineModelsCSIndirect& operator=(const PipelineModelsCSIndirect&) = delete;

	PipelineModelsCSIndirect(PipelineModelsCSIndirect&& other) noexcept
		: PipelineModelsBase{ std::move(other) },
		m_perPipelineSharedData{ other.m_perPipelineSharedData },
		m_perModelSharedData{ other.m_perModelSharedData },
		m_argumentInputSharedData{ std::move(other.m_argumentInputSharedData) }
	{}
	PipelineModelsCSIndirect& operator=(PipelineModelsCSIndirect&& other) noexcept
	{
		PipelineModelsBase::operator=(std::move(other));
		m_perPipelineSharedData     = other.m_perPipelineSharedData;
		m_perModelSharedData        = other.m_perModelSharedData;
		m_argumentInputSharedData   = std::move(other.m_argumentInputSharedData);

		return *this;
	}
};

class PipelineModelsVSIndirect
{
public:
	PipelineModelsVSIndirect();

	void CleanupData() noexcept { operator=(PipelineModelsVSIndirect{}); }

	void AllocateBuffers(
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers, UINT modelCount
	);
	void Draw(
		size_t frameIndex, ID3D12CommandSignature* commandSignature, const D3DCommandList& graphicsList
	) const noexcept;

	void RelinquishMemory(
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	) noexcept;

	void SetPSOIndex(std::uint32_t index) noexcept { m_psoIndex = index; }
	[[nodiscard]]
	std::uint32_t GetPSOIndex() const noexcept { return m_psoIndex; }

	[[nodiscard]]
	UINT GetModelCount() const noexcept { return m_modelCount; }

	[[nodiscard]]
	static consteval UINT64 GetCounterBufferSize() noexcept { return s_counterBufferSize; }
	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept { return 1u; }

private:
	UINT                          m_modelCount;
	std::uint32_t                 m_psoIndex;
	std::vector<SharedBufferData> m_argumentOutputSharedData;
	std::vector<SharedBufferData> m_counterSharedData;

	inline static UINT64 s_counterBufferSize = static_cast<UINT64>(sizeof(std::uint32_t));

public:
	PipelineModelsVSIndirect(const PipelineModelsVSIndirect&) = delete;
	PipelineModelsVSIndirect& operator=(const PipelineModelsVSIndirect&) = delete;

	PipelineModelsVSIndirect(PipelineModelsVSIndirect&& other) noexcept
		: m_modelCount{ other.m_modelCount }, m_psoIndex{ other.m_psoIndex },
		m_argumentOutputSharedData{ std::move(other.m_argumentOutputSharedData) },
		m_counterSharedData{ std::move(other.m_counterSharedData) }
	{}
	PipelineModelsVSIndirect& operator=(PipelineModelsVSIndirect&& other) noexcept
	{
		m_modelCount               = other.m_modelCount;
		m_psoIndex                 = other.m_psoIndex;
		m_argumentOutputSharedData = std::move(other.m_argumentOutputSharedData);
		m_counterSharedData        = std::move(other.m_counterSharedData);

		return *this;
	}
};

template<typename Pipeline_t>
class ModelBundleBase
{
public:
	ModelBundleBase()
		: m_pipelines{}, m_localModelIndexMap{}, m_modelBundle{}, m_modelBufferIndices{}
	{}

	[[nodiscard]]
	std::optional<size_t> GetPipelineLocalIndex(std::uint32_t pipelineIndex) const noexcept
	{
		return FindPipeline(pipelineIndex);
	}

	void SetModelIndices(std::vector<std::uint32_t> modelBufferIndices) noexcept
	{
		m_modelBufferIndices = std::move(modelBufferIndices);
	}

	void SetModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) noexcept
	{
		m_modelBundle = std::move(modelBundle);
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept
	{
		return m_modelBundle->GetMeshBundleIndex();
	}

	[[nodiscard]]
	std::uint32_t GetModelCount() const noexcept
	{
		return static_cast<std::uint32_t>(std::size(m_modelBufferIndices));
	}

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetModelBufferIndices() const noexcept
	{
		return m_modelBufferIndices;
	}

	[[nodiscard]]
	std::vector<std::uint32_t>&& TakeModelBufferIndices() noexcept
	{
		return std::move(m_modelBufferIndices);
	}

protected:
	using LocalIndexMap_t = std::unordered_map<std::uint32_t, std::uint32_t>;

	struct MoveModelCommonData
	{
		PipelineModelsBase::ModelData removedModelData;
		std::uint32_t                 newLocalPipelineIndex;
	};

protected:
	[[nodiscard]]
	std::optional<size_t> FindPipeline(std::uint32_t pipelineIndex) const noexcept
	{
		std::optional<size_t> index{};

		auto result = std::ranges::find(
			m_pipelines, pipelineIndex, [](const Pipeline_t& pipeline)
			{
				return pipeline.GetPSOIndex();
			}
		);

		if (result != std::end(m_pipelines))
			index = std::distance(std::begin(m_pipelines), result);

		return index;
	}

	void _cleanupData() noexcept { operator=(ModelBundleBase{}); }

	[[nodiscard]]
	size_t _addPipeline(std::uint32_t pipelineIndex)
	{
		const size_t pipelineLocalIndex = m_pipelines.Add(Pipeline_t{});

		if (pipelineLocalIndex >= std::size(m_localModelIndexMap))
			m_localModelIndexMap.emplace_back(LocalIndexMap_t{});

		Pipeline_t& pipeline = m_pipelines[pipelineLocalIndex];

		pipeline.SetPSOIndex(pipelineIndex);

		return pipelineLocalIndex;
	}

	void _removePipeline(size_t pipelineLocalIndex) noexcept
	{
		m_pipelines[pipelineLocalIndex].CleanupData();

		m_pipelines.RemoveElement(pipelineLocalIndex);
	}

	template<bool Sorted>
	void _addModelToPipeline(std::uint32_t pipelineLocalIndex, std::uint32_t modelIndex)
	{
		Pipeline_t& pipeline            = m_pipelines[pipelineLocalIndex];

		const std::uint32_t bufferIndex = m_modelBufferIndices[modelIndex];

		auto modelPipelineIndex         = std::numeric_limits<std::uint32_t>::max();

		if constexpr (Sorted)
			modelPipelineIndex = pipeline.AddModelSorted(modelIndex, bufferIndex);
		else
			modelPipelineIndex = pipeline.AddModel(modelIndex, bufferIndex);

		LocalIndexMap_t& pipelineMap = m_localModelIndexMap[pipelineLocalIndex];

		pipelineMap.insert_or_assign(modelIndex, modelPipelineIndex);
	}

	template<bool Sorted>
	void _addModel(std::uint32_t pipelineIndex, std::uint32_t modelIndex)
	{
		std::optional<size_t> oPipelineLocalIndex = FindPipeline(pipelineIndex);
		size_t pipelineLocalIndex                 = std::numeric_limits<size_t>::max();

		if (oPipelineLocalIndex)
			pipelineLocalIndex = oPipelineLocalIndex.value();
		else
			pipelineLocalIndex = _addPipeline(pipelineIndex);

		_addModelToPipeline<Sorted>(static_cast<std::uint32_t>(pipelineLocalIndex), modelIndex);
	}

	template<bool Sorted>
	void _addModels(std::uint32_t pipelineIndex, const std::vector<std::uint32_t>& modelIndices)
	{
		const size_t modelCount = std::size(modelIndices);

		for (size_t index = 0u; index < modelCount; ++index)
		{
			const std::uint32_t modelIndex = modelIndices[index];

			_addModel<Sorted>(pipelineIndex, modelIndex);
		}
	}

	template<bool Sorted>
	PipelineModelsBase::ModelData _removeModelFromPipeline(
		std::uint32_t pipelineLocalIndex, std::uint32_t modelIndex
	) noexcept {
		PipelineModelsBase::ModelData modelData
		{
			.bundleIndex = std::numeric_limits<std::uint32_t>::max(),
			.bufferIndex = std::numeric_limits<std::uint32_t>::max()
		};

		Pipeline_t& pipeline         = m_pipelines[pipelineLocalIndex];
		LocalIndexMap_t& pipelineMap = m_localModelIndexMap[pipelineLocalIndex];

		auto result                  = pipelineMap.find(modelIndex);

		if (result != std::end(pipelineMap))
		{
			const std::uint32_t modelLocalIndex = result->second;

			modelData = pipeline.GetModelData(modelLocalIndex);

			if constexpr (Sorted)
				pipeline.RemoveModelSorted(modelLocalIndex);
			else
				pipeline.RemoveModel(modelLocalIndex);

			pipelineMap.erase(modelIndex);
		}

		return modelData;
	}

	template<bool Sorted>
	PipelineModelsBase::ModelData _removeModel(
		std::uint32_t pipelineIndex, std::uint32_t modelIndex
	) noexcept {
		std::optional<size_t> oPipelineLocalIndex = FindPipeline(pipelineIndex);

		PipelineModelsBase::ModelData modelData
		{
			.bundleIndex = std::numeric_limits<std::uint32_t>::max(),
			.bufferIndex = std::numeric_limits<std::uint32_t>::max()
		};

		if (oPipelineLocalIndex)
		{
			const auto pipelineLocalIndex = static_cast<std::uint32_t>(oPipelineLocalIndex.value());

			modelData = _removeModelFromPipeline<Sorted>(pipelineLocalIndex, modelIndex);
		}

		return modelData;
	}

	template<bool Sorted>
	MoveModelCommonData _moveModelCommon(
		std::uint32_t modelIndex, std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) {
		auto oldPipelineLocalIndex = std::numeric_limits<std::uint32_t>::max();
		auto newPipelineLocalIndex = std::numeric_limits<std::uint32_t>::max();

		// Find the pipelines
		const size_t pipelineCount = std::size(m_pipelines);

		for (size_t index = 0u; index < pipelineCount; ++index)
		{
			const Pipeline_t& pipeline = m_pipelines[index];

			if (pipeline.GetPSOIndex() == oldPipelineIndex)
				oldPipelineLocalIndex = static_cast<std::uint32_t>(index);

			if (pipeline.GetPSOIndex() == newPipelineIndex)
				newPipelineLocalIndex = static_cast<std::uint32_t>(index);

			const bool bothFound = oldPipelineLocalIndex != std::numeric_limits<std::uint32_t>::max() &&
				newPipelineLocalIndex != std::numeric_limits<std::uint32_t>::max();

			if (bothFound)
				break;
		}

		// Move the model
		PipelineModelsBase::ModelData modelData
		{
			.bundleIndex = std::numeric_limits<std::uint32_t>::max(),
			.bufferIndex = std::numeric_limits<std::uint32_t>::max()
		};

		// If we can't find the old pipeline, then there will be nothing to move?
		if (oldPipelineLocalIndex != std::numeric_limits<std::uint32_t>::max())
			modelData = _removeModelFromPipeline<Sorted>(oldPipelineLocalIndex, modelIndex);

		return MoveModelCommonData
		{
			.removedModelData      = modelData,
			.newLocalPipelineIndex = newPipelineLocalIndex
		};
	}

	template<bool Sorted>
	void _moveModel(
		std::uint32_t modelIndex, std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) {
		const MoveModelCommonData moveData = _moveModelCommon<Sorted>(
			modelIndex, oldPipelineIndex, newPipelineIndex
		);

		std::uint32_t newPipelineLocalIndex            = moveData.newLocalPipelineIndex;
		const PipelineModelsBase::ModelData& modelData = moveData.removedModelData;

		// I feel like the new pipeline being not there already should be fine and we should add it.
		if (newPipelineLocalIndex == std::numeric_limits<std::uint32_t>::max())
			newPipelineLocalIndex = static_cast<std::uint32_t>(_addPipeline(newPipelineIndex));

		if (modelData.bundleIndex != std::numeric_limits<std::uint32_t>::max())
			_addModelToPipeline<Sorted>(newPipelineLocalIndex, modelData.bundleIndex);
	}

protected:
	ReusableVector<Pipeline_t>   m_pipelines;
	std::vector<LocalIndexMap_t> m_localModelIndexMap;
	std::shared_ptr<ModelBundle> m_modelBundle;
	std::vector<std::uint32_t>   m_modelBufferIndices;

public:
	ModelBundleBase(const ModelBundleBase&) = delete;
	ModelBundleBase& operator=(const ModelBundleBase&) = delete;

	ModelBundleBase(ModelBundleBase&& other) noexcept
		: m_pipelines{ std::move(other.m_pipelines) },
		m_localModelIndexMap{ std::move(other.m_localModelIndexMap) },
		m_modelBundle{ std::move(other.m_modelBundle) },
		m_modelBufferIndices{ std::move(other.m_modelBufferIndices) }
	{
	}
	ModelBundleBase& operator=(ModelBundleBase&& other) noexcept
	{
		m_pipelines          = std::move(other.m_pipelines);
		m_localModelIndexMap = std::move(other.m_localModelIndexMap);
		m_modelBundle        = std::move(other.m_modelBundle);
		m_modelBufferIndices = std::move(other.m_modelBufferIndices);

		return *this;
	}
};

template<typename Pipeline_t>
class ModelBundleCommon : public ModelBundleBase<Pipeline_t>
{
public:
	ModelBundleCommon() : ModelBundleBase<Pipeline_t>{} {}

	[[nodiscard]]
	std::uint32_t AddPipeline(std::uint32_t pipelineIndex)
	{
		return static_cast<std::uint32_t>(this->_addPipeline(pipelineIndex));
	}

	void RemovePipeline(size_t pipelineLocalIndex) noexcept
	{
		this->_removePipeline(pipelineLocalIndex);
	}

	void CleanupData() noexcept { this->_cleanupData(); }

	// Model Index is the index in the ModelBundle and bufferIndex would be the
	// index in the GPU visible buffer.
	void AddModel(std::uint32_t pipelineIndex, std::uint32_t modelIndex)
	{
		this->_addModel<false>(pipelineIndex, modelIndex);
	}
	void AddModelSorted(std::uint32_t pipelineIndex, std::uint32_t modelIndex)
	{
		this->_addModel<true>(pipelineIndex, modelIndex);
	}

	// The size of both of the vectors must be the same.
	void AddModels(std::uint32_t pipelineIndex, const std::vector<std::uint32_t>& modelIndices)
	{
		this->_addModels<false>(pipelineIndex, modelIndices);
	}
	void AddModelsSorted(std::uint32_t pipelineIndex, const std::vector<std::uint32_t>& modelIndices)
	{
		this->_addModels<true>(pipelineIndex, modelIndices);
	}

	void MoveModel(
		std::uint32_t modelIndex, std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) {
		this->_moveModel<false>(modelIndex, oldPipelineIndex, newPipelineIndex);
	}
	void MoveModelSorted(
		std::uint32_t modelIndex, std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) {
		this->_moveModel<true>(modelIndex, oldPipelineIndex, newPipelineIndex);
	}

	void RemoveModel(std::uint32_t pipelineIndex, std::uint32_t modelIndex) noexcept
	{
		this->_removeModel<false>(pipelineIndex, modelIndex);
	}
	void RemoveModelSorted(std::uint32_t pipelineIndex, std::uint32_t modelIndex) noexcept
	{
		this->_removeModel<true>(pipelineIndex, modelIndex);
	}

public:
	ModelBundleCommon(const ModelBundleCommon&) = delete;
	ModelBundleCommon& operator=(const ModelBundleCommon&) = delete;

	ModelBundleCommon(ModelBundleCommon&& other) noexcept
		: ModelBundleBase<Pipeline_t>{ std::move(other) }
	{}
	ModelBundleCommon& operator=(ModelBundleCommon&& other) noexcept
	{
		ModelBundleBase<Pipeline_t>::operator=(std::move(other));

		return *this;
	}
};

class ModelBundleVSIndividual : public ModelBundleCommon<PipelineModelsVSIndividual>
{
	using GraphicsPipeline_t = GraphicsPipelineVSIndividualDraw;

public:
	ModelBundleVSIndividual() : ModelBundleCommon{} {}

	void Draw(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle,
		const PipelineManager<GraphicsPipeline_t>& pipelineManager
	) const noexcept;
	void DrawSorted(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle,
		const PipelineManager<GraphicsPipeline_t>& pipelineManager
	) noexcept;

	void DrawPipeline(
		size_t pipelineLocalIndex, const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleVS& meshBundle
	) const noexcept;
	void DrawPipelineSorted(
		size_t pipelineLocalIndex, const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleVS& meshBundle
	) noexcept;

public:
	ModelBundleVSIndividual(const ModelBundleVSIndividual&) = delete;
	ModelBundleVSIndividual& operator=(const ModelBundleVSIndividual&) = delete;

	ModelBundleVSIndividual(ModelBundleVSIndividual&& other) noexcept
		: ModelBundleCommon{ std::move(other) }
	{}
	ModelBundleVSIndividual& operator=(ModelBundleVSIndividual&& other) noexcept
	{
		ModelBundleCommon::operator=(std::move(other));

		return *this;
	}
};

class ModelBundleMSIndividual : public ModelBundleCommon<PipelineModelsMSIndividual>
{
	using GraphicsPipeline_t = GraphicsPipelineMS;

public:
	ModelBundleMSIndividual() : ModelBundleCommon{} {}

	void Draw(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle,
		const PipelineManager<GraphicsPipeline_t>& pipelineManager
	) const noexcept;
	void DrawSorted(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle,
		const PipelineManager<GraphicsPipeline_t>& pipelineManager
	) noexcept;

	void DrawPipeline(
		size_t pipelineLocalIndex, const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleMS& meshBundle
	) const noexcept;
	void DrawPipelineSorted(
		size_t pipelineLocalIndex, const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleMS& meshBundle
	) noexcept;

private:
	static void SetMeshBundleConstants(
		ID3D12GraphicsCommandList* graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleMS& meshBundle
	) noexcept;

public:
	ModelBundleMSIndividual(const ModelBundleMSIndividual&) = delete;
	ModelBundleMSIndividual& operator=(const ModelBundleMSIndividual&) = delete;

	ModelBundleMSIndividual(ModelBundleMSIndividual&& other) noexcept
		: ModelBundleCommon{ std::move(other) }
	{}
	ModelBundleMSIndividual& operator=(ModelBundleMSIndividual&& other) noexcept
	{
		ModelBundleCommon::operator=(std::move(other));

		return *this;
	}
};

class ModelBundleVSIndirect : public ModelBundleBase<PipelineModelsCSIndirect>
{
	using GraphicsPipeline_t = GraphicsPipelineVSIndirectDraw;

public:
	ModelBundleVSIndirect() : ModelBundleBase{}, m_vsPipelines{} {}

	[[nodiscard]]
	std::uint32_t AddPipeline(std::uint32_t pipelineIndex);

	void RemovePipeline(
		size_t pipelineLocalIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	) noexcept;

	void CleanupData(
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	) noexcept;

	// Model Index is the index in the ModelBundle and bufferIndex would be the
	// index in the GPU visible buffer.
	void AddModel(
		std::uint32_t pipelineIndex, std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);
	void AddModelSorted(
		std::uint32_t pipelineIndex, std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	// The size of both of the index vectors must be the same.
	void AddModels(
		std::uint32_t pipelineIndex, std::uint32_t modelBundleIndex,
		const std::vector<std::uint32_t>& modelIndices,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);
	void AddModelsSorted(
		std::uint32_t pipelineIndex, std::uint32_t modelBundleIndex,
		const std::vector<std::uint32_t>& modelIndices,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	void MoveModel(
		std::uint32_t modelIndex, std::uint32_t modelBundleIndex,
		std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);
	void MoveModelSorted(
		std::uint32_t modeIndex, std::uint32_t modelBundleIndex,
		std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	void RemoveModel(std::uint32_t pipelineIndex, std::uint32_t modelIndex) noexcept
	{
		_removeModel<false>(pipelineIndex, modelIndex);
	}
	void RemoveModelSorted(std::uint32_t pipelineIndex, std::uint32_t modelIndex) noexcept
	{
		_removeModel<true>(pipelineIndex, modelIndex);
	}

	void Update(size_t frameIndex, const D3DMeshBundleVS& meshBundle) const noexcept;
	void UpdateSorted(size_t frameIndex, const D3DMeshBundleVS& meshBundle) noexcept;

	void UpdatePipeline(
		size_t pipelineLocalIndex, size_t frameIndex, const D3DMeshBundleVS& meshBundle
	) const noexcept;
	void UpdatePipelineSorted(
		size_t pipelineLocalIndex, size_t frameIndex, const D3DMeshBundleVS& meshBundle
	) noexcept;

	void Draw(
		size_t frameIndex, const D3DCommandList& graphicsList, ID3D12CommandSignature* commandSignature,
		const D3DMeshBundleVS& meshBundle, const PipelineManager<GraphicsPipeline_t>& pipelineManager
	) const noexcept;

	void DrawPipeline(
		size_t pipelineLocalIndex, size_t frameIndex, const D3DCommandList& graphicsList,
		ID3D12CommandSignature* commandSignature, const D3DMeshBundleVS& meshBundle
	) const noexcept;

private:
	void _addModels(
		std::uint32_t pipelineIndex, std::uint32_t modelBundleIndex,
		std::uint32_t const* modelIndices, std::uint32_t const* bufferIndices,
		size_t modelCount, bool areModelsSorted,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);
	void _addModelsToPipeline(
		std::uint32_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
		std::uint32_t const* modelIndices, std::uint32_t const* bufferIndices,
		size_t modelCount, bool areModelsSorted,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	void ResizePreviousPipelines(
		size_t addableStartIndex, size_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
		bool areModelsSorted, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	void RecreateFollowingPipelines(
		size_t pipelineLocalIndex, std::uint32_t modelBundleIndex, bool areModelsSorted,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	void _moveModel(
		std::uint32_t modelIndex, std::uint32_t modelBundleIndex,
		std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex, bool areModelsSorted,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	[[nodiscard]]
	size_t GetLocalPipelineIndex(std::uint32_t pipelineIndex) noexcept;
	[[nodiscard]]
	size_t FindAddableStartIndex(size_t pipelineLocalIndex, size_t modelCount) const noexcept;

private:
	std::vector<PipelineModelsVSIndirect> m_vsPipelines;

public:
	ModelBundleVSIndirect(const ModelBundleVSIndirect&) = delete;
	ModelBundleVSIndirect& operator=(const ModelBundleVSIndirect&) = delete;

	ModelBundleVSIndirect(ModelBundleVSIndirect&& other) noexcept
		: ModelBundleBase{ std::move(other) },
		m_vsPipelines{ std::move(other.m_vsPipelines) }
	{}
	ModelBundleVSIndirect& operator=(ModelBundleVSIndirect&& other) noexcept
	{
		ModelBundleBase::operator=(std::move(other));
		m_vsPipelines            = std::move(other.m_vsPipelines);

		return *this;
	}
};
#endif
