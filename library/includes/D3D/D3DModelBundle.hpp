#ifndef D3D_MODEL_BUNDLE_HPP_
#define D3D_MODEL_BUNDLE_HPP_
#include <memory>
#include <vector>
#include <Model.hpp>
#include <ReusableVector.hpp>
#include <D3DMeshBundleMS.hpp>
#include <D3DMeshBundleVS.hpp>
#include <D3DGraphicsPipelineVS.hpp>
#include <D3DGraphicsPipelineMS.hpp>
#include <D3DPipelineManager.hpp>

namespace Gaia
{
class PipelineModelsBase
{
public:
	PipelineModelsBase() : m_localPipelineIndex{ 0u } {}

	[[nodiscard]]
	static D3D12_DRAW_INDEXED_ARGUMENTS GetDrawIndexedIndirectCommand(
		const MeshTemporaryDetailsVS& meshDetailsVS
	) noexcept;

	void SetPipelineIndex(std::uint32_t pipelineIndex) noexcept
	{
		m_localPipelineIndex = pipelineIndex;
	}

protected:
	void _cleanupData() noexcept { operator=(PipelineModelsBase{}); }

protected:
	std::uint32_t m_localPipelineIndex;

public:
	PipelineModelsBase(const PipelineModelsBase&) = delete;
	PipelineModelsBase& operator=(const PipelineModelsBase&) = delete;

	PipelineModelsBase(PipelineModelsBase&& other) noexcept
		: m_localPipelineIndex{ other.m_localPipelineIndex }
	{}
	PipelineModelsBase& operator=(PipelineModelsBase&& other) noexcept
	{
		m_localPipelineIndex = other.m_localPipelineIndex;

		return *this;
	}
};

class PipelineModelsVSIndividual: public PipelineModelsBase
{
public:
	PipelineModelsVSIndividual() : PipelineModelsBase{} {}

	void CleanupData() noexcept { _cleanupData(); }

	void Draw(
		const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleVS& meshBundle, const PipelineModelBundle& pipelineBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept { return 1u; }

private:
	void DrawModel(
		const std::shared_ptr<Model>& model, ID3D12GraphicsCommandList* graphicsList,
		UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle
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
		const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleMS& meshBundle, const PipelineModelBundle& pipelineBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

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

	void DrawModel(
		const std::shared_ptr<Model>& model, ID3D12GraphicsCommandList6* graphicsList,
		UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle
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
	enum class ModelFlag : std::uint32_t
	{
		Visibility  = 1u,
		SkipCulling = 2u
	};

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
		std::uint32_t modelFlags;
	};

public:
	PipelineModelsCSIndirect();

	void CleanupData() noexcept { operator=(PipelineModelsCSIndirect{}); }

	[[nodiscard]]
	size_t GetAddableModelCount(
		const std::vector<PipelineModelBundle>& pipelineBundles
	) const noexcept;
	[[nodiscard]]
	size_t GetNewModelCount(
		const std::vector<PipelineModelBundle>& pipelineBundles
	) const noexcept;

	void UpdateNonPerFrameData(
		std::uint32_t modelBundleIndex, const std::vector<PipelineModelBundle>& pipelineBundles
	) noexcept;

	void AllocateBuffers(
		const std::vector<PipelineModelBundle>& pipelineBundles,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer
	);

	void ResetCullingData() const noexcept;

	void Update(
		size_t frameIndex, const D3DMeshBundleVS& meshBundle, bool skipCulling,
		const PipelineModelBundle& pipelineBundle,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

	void RelinquishMemory(
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer
	) noexcept;

	[[nodiscard]]
	std::uint32_t GetModelCount(
		const std::vector<PipelineModelBundle>& pipelineBundles
	) const noexcept {
		return static_cast<std::uint32_t>(pipelineBundles[m_localPipelineIndex].GetModelCount());
	}

	[[nodiscard]]
	static consteval size_t GetPerModelStride() noexcept
	{
		return sizeof(PerModelData);
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

	void SetModelCount(UINT count) noexcept { m_modelCount = count; }

	void AllocateBuffers(
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);
	void Draw(
		size_t frameIndex, ID3D12CommandSignature* commandSignature,
		const D3DCommandList& graphicsList
	) const noexcept;

	void RelinquishMemory(
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	) noexcept;

	[[nodiscard]]
	static consteval UINT64 GetCounterBufferSize() noexcept { return s_counterBufferSize; }
	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept { return 1u; }

private:
	std::vector<SharedBufferData> m_argumentOutputSharedData;
	std::vector<SharedBufferData> m_counterSharedData;
	UINT                          m_modelCount;

	inline static UINT64 s_counterBufferSize = static_cast<UINT64>(sizeof(std::uint32_t));

public:
	PipelineModelsVSIndirect(const PipelineModelsVSIndirect&) = delete;
	PipelineModelsVSIndirect& operator=(const PipelineModelsVSIndirect&) = delete;

	PipelineModelsVSIndirect(PipelineModelsVSIndirect&& other) noexcept
		: m_argumentOutputSharedData{ std::move(other.m_argumentOutputSharedData) },
		m_counterSharedData{ std::move(other.m_counterSharedData) },
		m_modelCount{ other.m_modelCount }
	{}
	PipelineModelsVSIndirect& operator=(PipelineModelsVSIndirect&& other) noexcept
	{
		m_argumentOutputSharedData = std::move(other.m_argumentOutputSharedData);
		m_counterSharedData        = std::move(other.m_counterSharedData);
		m_modelCount               = other.m_modelCount;

		return *this;
	}
};

template<typename Pipeline_t>
class ModelBundleBase
{
public:
	ModelBundleBase() : m_pipelines{}, m_modelBundle{} {}

	[[nodiscard]]
	std::optional<size_t> GetPipelineLocalIndex(std::uint32_t pipelineIndex) const noexcept
	{
		return m_modelBundle->FindLocalPipelineIndex(pipelineIndex);
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
		return static_cast<std::uint32_t>(std::size(m_modelBundle->GetModels()));
	}

	[[nodiscard]]
	const std::shared_ptr<ModelBundle>& GetModelBundle() const noexcept { return m_modelBundle; }

protected:
	void _cleanupData() noexcept { operator=(ModelBundleBase{}); }

	size_t _addPipeline(std::uint32_t localIndex)
	{
		Pipeline_t pipeline{};

		pipeline.SetPipelineIndex(localIndex);

		return m_pipelines.Add(std::move(pipeline));
	}

	void _removePipeline(size_t pipelineLocalIndex) noexcept
	{
		m_pipelines[pipelineLocalIndex].CleanupData();

		m_pipelines.RemoveElement(pipelineLocalIndex);
	}

protected:
	Callisto::ReusableVector<Pipeline_t> m_pipelines;
	std::shared_ptr<ModelBundle>         m_modelBundle;

public:
	ModelBundleBase(const ModelBundleBase&) = delete;
	ModelBundleBase& operator=(const ModelBundleBase&) = delete;

	ModelBundleBase(ModelBundleBase&& other) noexcept
		: m_pipelines{ std::move(other.m_pipelines) },
		m_modelBundle{ std::move(other.m_modelBundle) }
	{
	}
	ModelBundleBase& operator=(ModelBundleBase&& other) noexcept
	{
		m_pipelines   = std::move(other.m_pipelines);
		m_modelBundle = std::move(other.m_modelBundle);

		return *this;
	}
};

template<typename Pipeline_t>
class ModelBundleCommon : public ModelBundleBase<Pipeline_t>
{
public:
	ModelBundleCommon() : ModelBundleBase<Pipeline_t>{} {}

	// Assuming any new pipelines will added at the back.
	void AddNewPipelinesFromBundle()
	{
		const size_t pipelinesInBundle    = this->m_modelBundle->GetPipelineCount();
		const size_t currentPipelineCount = std::size(this->m_pipelines);

		for (size_t index = currentPipelineCount; index < pipelinesInBundle; ++index)
			this->_addPipeline(static_cast<std::uint32_t>(index));
	}

	void RemovePipeline(size_t pipelineLocalIndex) noexcept
	{
		this->_removePipeline(pipelineLocalIndex);
	}

	void CleanupData() noexcept { this->_cleanupData(); }

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

	void DrawPipeline(
		size_t pipelineLocalIndex, const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleVS& meshBundle
	) const noexcept;

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

	void DrawPipeline(
		size_t pipelineLocalIndex, const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const D3DMeshBundleMS& meshBundle
	) const noexcept;

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

	// Assuming any new pipelines will added at the back.
	void AddNewPipelinesFromBundle(
		std::uint32_t modelBundleIndex, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	void RemovePipeline(
		size_t pipelineLocalIndex, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
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

	void ReconfigureModels(
		std::uint32_t modelBundleIndex, std::uint32_t decreasedModelsPipelineIndex,
		std::uint32_t increasedModelsPipelineIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	void UpdatePipeline(
		size_t pipelineLocalIndex, size_t frameIndex, const D3DMeshBundleVS& meshBundle,
		bool skipCulling
	) const noexcept;

	void DrawPipeline(
		size_t pipelineLocalIndex, size_t frameIndex, const D3DCommandList& graphicsList,
		ID3D12CommandSignature* commandSignature, const D3DMeshBundleVS& meshBundle
	) const noexcept;

	void SetupPipelineBuffers(
		std::uint32_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

private:
	void ResizePreviousPipelines(
		size_t addableStartIndex, size_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	void RecreateFollowingPipelines(
		size_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
	);

	[[nodiscard]]
	size_t FindAddableStartIndex(size_t pipelineLocalIndex, size_t modelCount) const noexcept;

	[[nodiscard]]
	size_t GetLocalPipelineIndex(std::uint32_t pipelineIndex) noexcept;

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
}
#endif
