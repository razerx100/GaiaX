#ifndef D3D_MODEL_BUNDLE_HPP_
#define D3D_MODEL_BUNDLE_HPP_
#include <memory>
#include <vector>
#include <Model.hpp>
#include <D3DMeshBundleMS.hpp>
#include <D3DMeshBundleVS.hpp>

class ModelBundleBase
{
public:
	ModelBundleBase() : m_psoIndex{ 0u } {}

	void SetPSOIndex(std::uint32_t index) noexcept { m_psoIndex = index; }

	[[nodiscard]]
	std::uint32_t GetPSOIndex() const noexcept { return m_psoIndex; }

	[[nodiscard]]
	static D3D12_DRAW_INDEXED_ARGUMENTS GetDrawIndexedIndirectCommand(
		const MeshTemporaryDetailsVS& meshDetailsVS
	) noexcept;

protected:
	std::uint32_t m_psoIndex;

public:
	ModelBundleBase(const ModelBundleBase&) = delete;
	ModelBundleBase& operator=(const ModelBundleBase&) = delete;

	ModelBundleBase(ModelBundleBase&& other) noexcept
		: m_psoIndex{ other.m_psoIndex }
	{}
	ModelBundleBase& operator=(ModelBundleBase&& other) noexcept
	{
		m_psoIndex = other.m_psoIndex;

		return *this;
	}
};

class ModelBundleVSIndividual : public ModelBundleBase
{
public:
	ModelBundleVSIndividual() : ModelBundleBase{}, m_modelBufferIndices{}, m_modelBundle{} {}

	void SetModelBundle(
		std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
	) noexcept;
	void Draw(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle
	) const noexcept;

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept { return 1u; }

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetModelIndices() const noexcept { return m_modelBufferIndices; }

	[[nodiscard]]
	std::uint32_t GetID() const noexcept
	{
		if (!std::empty(m_modelBufferIndices))
			return m_modelBufferIndices.front();
		else
			return std::numeric_limits<std::uint32_t>::max();
	}

private:
	std::vector<std::uint32_t>   m_modelBufferIndices;
	std::shared_ptr<ModelBundle> m_modelBundle;

public:
	ModelBundleVSIndividual(const ModelBundleVSIndividual&) = delete;
	ModelBundleVSIndividual& operator=(const ModelBundleVSIndividual&) = delete;

	ModelBundleVSIndividual(ModelBundleVSIndividual&& other) noexcept
		: ModelBundleBase{ std::move(other) },
		m_modelBufferIndices{ std::move(other.m_modelBufferIndices) },
		m_modelBundle{ std::move(other.m_modelBundle) }
	{}
	ModelBundleVSIndividual& operator=(ModelBundleVSIndividual&& other) noexcept
	{
		ModelBundleBase::operator=(std::move(other));
		m_modelBufferIndices     = std::move(other.m_modelBufferIndices);
		m_modelBundle            = std::move(other.m_modelBundle);

		return *this;
	}
};

class ModelBundleMSIndividual : public ModelBundleBase
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
	ModelBundleMSIndividual() : ModelBundleBase{}, m_modelBundle{}, m_modelBufferIndices{} {}

	void SetModelBundle(
		std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
	) noexcept;

	void Draw(
		const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle
	) const noexcept;

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept
	{
		return static_cast<UINT>(sizeof(ModelDetailsMS) / sizeof(UINT));
	}

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetModelIndices() const noexcept { return m_modelBufferIndices; }
	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }

	[[nodiscard]]
	std::uint32_t GetID() const noexcept
	{
		if (!std::empty(m_modelBufferIndices))
			return m_modelBufferIndices.front();
		else
			return std::numeric_limits<std::uint32_t>::max();
	}

private:
	[[nodiscard]]
	static UINT DivRoundUp(UINT num, UINT den) noexcept
	{
		return (num + den - 1) / den;
	}

private:
	std::shared_ptr<ModelBundle> m_modelBundle;
	std::vector<std::uint32_t>   m_modelBufferIndices;

	static constexpr UINT s_amplificationLaneCount = 32u;

public:
	ModelBundleMSIndividual(const ModelBundleMSIndividual&) = delete;
	ModelBundleMSIndividual& operator=(const ModelBundleMSIndividual&) = delete;

	ModelBundleMSIndividual(ModelBundleMSIndividual&& other) noexcept
		: ModelBundleBase{ std::move(other) },
		m_modelBundle{ std::move(other.m_modelBundle) },
		m_modelBufferIndices{ std::move(other.m_modelBufferIndices) }
	{}
	ModelBundleMSIndividual& operator=(ModelBundleMSIndividual&& other) noexcept
	{
		ModelBundleBase::operator=(std::move(other));
		m_modelBundle            = std::move(other.m_modelBundle);
		m_modelBufferIndices     = std::move(other.m_modelBufferIndices);

		return *this;
	}
};

class ModelBundleCSIndirect
{
public:
	struct CullingData
	{
		std::uint32_t commandCount;
		std::uint32_t commandOffset;// Next Vec2 starts at 8bytes offset
	};

	struct IndirectArgument
	{
		UINT                         modelIndex;
		D3D12_DRAW_INDEXED_ARGUMENTS drawArguments;
	};

public:
	ModelBundleCSIndirect();

	void SetModelBundle(std::shared_ptr<ModelBundle> bundle) noexcept;
	void CreateBuffers(
		StagingBufferManager& stagingBufferMan,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffer,
		SharedBufferCPU& cullingSharedBuffer, SharedBufferGPU& perModelDataSharedBuffer,
		std::vector<std::uint32_t> modelIndices, TemporaryDataBufferGPU& tempBuffer
	);

	void Update(size_t bufferIndex, const D3DMeshBundleVS& meshBundle) const noexcept;

	void ResetCullingData() const noexcept;

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }

	[[nodiscard]]
	std::uint32_t GetID() const noexcept
	{
		if (!std::empty(m_modelIndices))
			return m_modelIndices.front();
		else
			return std::numeric_limits<std::uint32_t>::max();
	}
	[[nodiscard]]
	// Must be called after the buffers have been created.
	std::uint32_t GetModelBundleIndex() const noexcept
	{
		return static_cast<std::uint32_t>(m_cullingSharedData.offset / sizeof(CullingData));
	}
	[[nodiscard]]
	UINT GetModelCount() const noexcept
	{
		return static_cast<UINT>(std::size(m_modelIndices));
	}
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetModelIndices() const noexcept { return m_modelIndices; }

	[[nodiscard]]
	const std::vector<SharedBufferData>& GetArgumentInputSharedData() const noexcept
	{
		return m_argumentInputSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetCullingSharedData() const noexcept { return m_cullingSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetPerModelDataSharedData() const noexcept
	{
		return m_perModelSharedData;
	}

private:
	SharedBufferData              m_perModelSharedData;
	SharedBufferData              m_cullingSharedData;
	std::vector<SharedBufferData> m_argumentInputSharedData;
	std::vector<std::uint32_t>    m_modelIndices;
	std::shared_ptr<ModelBundle>  m_modelBundle;

public:
	ModelBundleCSIndirect(const ModelBundleCSIndirect&) = delete;
	ModelBundleCSIndirect& operator=(const ModelBundleCSIndirect&) = delete;

	ModelBundleCSIndirect(ModelBundleCSIndirect&& other) noexcept
		: m_perModelSharedData{ other.m_perModelSharedData },
		m_cullingSharedData{ other.m_cullingSharedData },
		m_argumentInputSharedData{ std::move(other.m_argumentInputSharedData) },
		m_modelIndices{ std::move(other.m_modelIndices) },
		m_modelBundle{ std::move(other.m_modelBundle) }
	{}
	ModelBundleCSIndirect& operator=(ModelBundleCSIndirect&& other) noexcept
	{
		m_perModelSharedData      = other.m_perModelSharedData;
		m_cullingSharedData       = other.m_cullingSharedData;
		m_argumentInputSharedData = std::move(other.m_argumentInputSharedData);
		m_modelIndices            = std::move(other.m_modelIndices);
		m_modelBundle             = std::move(other.m_modelBundle);

		return *this;
	}
};

class ModelBundleVSIndirect : public ModelBundleBase
{
public:
	ModelBundleVSIndirect();

	void SetModelBundle(std::shared_ptr<ModelBundle> bundle) noexcept;

	void CreateBuffers(
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers, UINT modelCount
	);
	void Draw(
		size_t frameIndex, ID3D12CommandSignature* commandSignature, const D3DCommandList& graphicsList
	) const noexcept;

	void SetID(std::uint32_t bundleID) noexcept { m_bundleID = bundleID; }
	[[nodiscard]]
	std::uint32_t GetID() const noexcept { return m_bundleID; }

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }
	[[nodiscard]]
	const std::vector<SharedBufferData>& GetArgumentOutputSharedData() const noexcept
	{
		return m_argumentOutputSharedData;
	}
	[[nodiscard]]
	const std::vector<SharedBufferData>& GetCounterSharedData() const noexcept
	{
		return m_counterSharedData;
	}

	[[nodiscard]]
	static UINT64 GetCounterBufferSize() noexcept { return s_counterBufferSize; }

	[[nodiscard]]
	static constexpr UINT GetConstantCount() noexcept { return 1u; }

private:
	UINT                          m_modelCount;
	std::shared_ptr<ModelBundle>  m_modelBundle;
	std::vector<SharedBufferData> m_argumentOutputSharedData;
	std::vector<SharedBufferData> m_counterSharedData;
	std::uint32_t                 m_bundleID;

	inline static UINT64 s_counterBufferSize = static_cast<UINT64>(sizeof(std::uint32_t));

public:
	ModelBundleVSIndirect(const ModelBundleVSIndirect&) = delete;
	ModelBundleVSIndirect& operator=(const ModelBundleVSIndirect&) = delete;

	ModelBundleVSIndirect(ModelBundleVSIndirect&& other) noexcept
		: ModelBundleBase{ std::move(other) },
		m_modelCount{ other.m_modelCount },
		m_modelBundle{ std::move(other.m_modelBundle) },
		m_argumentOutputSharedData{ std::move(other.m_argumentOutputSharedData) },
		m_counterSharedData{ std::move(other.m_counterSharedData) },
		m_bundleID{ other.m_bundleID }
	{}
	ModelBundleVSIndirect& operator=(ModelBundleVSIndirect&& other) noexcept
	{
		ModelBundleBase::operator=(std::move(other));
		m_modelCount               = other.m_modelCount;
		m_modelBundle              = std::move(other.m_modelBundle);
		m_argumentOutputSharedData = std::move(other.m_argumentOutputSharedData);
		m_counterSharedData        = std::move(other.m_counterSharedData);
		m_bundleID                 = other.m_bundleID;

		return *this;
	}
};
#endif
