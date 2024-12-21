#ifndef MODEL_MANAGER_HPP_
#define MODEL_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <utility>
#include <vector>
#include <ranges>
#include <algorithm>
#include <Model.hpp>
#include <D3DCommandQueue.hpp>
#include <MeshManagerMeshShader.hpp>
#include <ReusableD3DBuffer.hpp>
#include <GraphicsPipelineVertexShader.hpp>
#include <GraphicsPipelineMeshShader.hpp>
#include <ComputePipeline.hpp>
#include <MeshManagerMeshShader.hpp>
#include <MeshManagerVertexShader.hpp>

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
		const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const MeshManagerVertexShader& meshBundle
	) const noexcept;

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept { return 1u; }

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetIndices() const noexcept { return m_modelBufferIndices; }

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
		const D3DCommandList& graphicsList, UINT constantsRootIndex,
		const MeshManagerMeshShader& meshBundle
	) const noexcept;

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept
	{
		return static_cast<UINT>(sizeof(ModelDetailsMS) / sizeof(UINT));
	}

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetIndices() const noexcept { return m_modelBufferIndices; }
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

	void Update(size_t bufferIndex, const MeshManagerVertexShader& meshBundle) const noexcept;

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

class ModelBuffers : public ReusableD3DBuffer<ModelBuffers, std::shared_ptr<Model>>
{
	friend class ReusableD3DBuffer<ModelBuffers, std::shared_ptr<Model>>;

public:
	ModelBuffers(
		ID3D12Device* device, MemoryManager* memoryManager, std::uint32_t frameCount
	) : ReusableD3DBuffer{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
		m_pixelModelBuffers{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
		m_modelBuffersInstanceSize{ 0u }, m_modelBuffersPixelInstanceSize{ 0u },
		m_bufferInstanceCount{ frameCount }
	{}

	void SetDescriptor(
		D3DDescriptorManager& descriptorManager, UINT64 frameIndex, size_t registerSlot,
		size_t registerSpace, bool graphicsQueue
	) const;
	void SetPixelDescriptor(
		D3DDescriptorManager& descriptorManager, UINT64 frameIndex, size_t registerSlot,
		size_t registerSpace
	) const;

	void Update(UINT64 bufferIndex) const noexcept;

	[[nodiscard]]
	std::uint32_t GetInstanceCount() const noexcept { return m_bufferInstanceCount; }

private:
	struct ModelVertexData
	{
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMFLOAT3 modelOffset;
		// The materialIndex must be grabbed from the z component.
		std::uint32_t     materialIndex;
		std::uint32_t     meshIndex;
		float             modelScale;
		// This struct is 16 bytes aligned thanks to the Matrix. So, putting the correct
		// amount of padding. Just to make it more obvious though, as it would have been
		// put anyway. Structured Buffer doesn't require any specific alignment.
		std::uint32_t     padding[2];
	};

	struct ModelPixelData
	{
		UVInfo        diffuseTexUVInfo;
		UVInfo        specularTexUVInfo;
		std::uint32_t diffuseTexIndex;
		std::uint32_t specularTexIndex;
		float         padding[2]; // Needs to be 16bytes aligned.
	};

private:
	[[nodiscard]]
	static consteval size_t GetVertexStride() noexcept { return sizeof(ModelVertexData); }
	[[nodiscard]]
	static consteval size_t GetPixelStride() noexcept { return sizeof(ModelPixelData); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t modelCount);

private:
	Buffer        m_pixelModelBuffers;
	UINT64        m_modelBuffersInstanceSize;
	UINT64        m_modelBuffersPixelInstanceSize;
	std::uint32_t m_bufferInstanceCount;

public:
	ModelBuffers(const ModelBuffers&) = delete;
	ModelBuffers& operator=(const ModelBuffers&) = delete;

	ModelBuffers(ModelBuffers&& other) noexcept
		: ReusableD3DBuffer{ std::move(other) },
		m_pixelModelBuffers{ std::move(other.m_pixelModelBuffers) },
		m_modelBuffersInstanceSize{ other.m_modelBuffersInstanceSize },
		m_modelBuffersPixelInstanceSize{ other.m_modelBuffersPixelInstanceSize },
		m_bufferInstanceCount{ other.m_bufferInstanceCount }
	{}
	ModelBuffers& operator=(ModelBuffers&& other) noexcept
	{
		ReusableD3DBuffer::operator=(std::move(other));
		m_pixelModelBuffers             = std::move(other.m_pixelModelBuffers);
		m_modelBuffersInstanceSize      = other.m_modelBuffersInstanceSize;
		m_modelBuffersPixelInstanceSize = other.m_modelBuffersPixelInstanceSize;
		m_bufferInstanceCount           = other.m_bufferInstanceCount;

		return *this;
	}
};

template<
	class Derived,
	class Pipeline,
	class MeshManager,
	class ModelBundleType
>
class ModelManager
{
public:
	ModelManager(
		ID3D12Device5* device, MemoryManager* memoryManager, std::uint32_t frameCount
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_graphicsRootSignature{ nullptr }, m_shaderPath{},
		m_modelBuffers{ device, memoryManager, frameCount },
		m_graphicsPipelines{}, m_meshBundles{}, m_modelBundles{}, m_oldBufferCopyNecessary{ false }
	{}

	void SetGraphicsRootSignature(ID3D12RootSignature* rootSignature) noexcept
	{
		m_graphicsRootSignature = rootSignature;
	}

	void SetGraphicsConstantsRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept {
		static_cast<Derived*>(this)->_setGraphicsConstantRootIndex(
			descriptorManager, constantsRegisterSpace
		);
	}

	void UpdatePerFrame(UINT64 frameIndex) const noexcept
	{
		m_modelBuffers.Update(frameIndex);

		static_cast<Derived const*>(this)->_updatePerFrame(frameIndex);
	}

	void SetShaderPath(std::wstring shaderPath)
	{
		m_shaderPath = std::move(shaderPath);

		static_cast<Derived*>(this)->ShaderPathSet();
	}

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader,
		TemporaryDataBufferGPU& tempBuffer
	) {
		const auto& models      = modelBundle->GetModels();
		const size_t modelCount = std::size(models);

		if (modelCount)
		{
			std::vector<std::shared_ptr<Model>> tempModelBundle{ modelCount, nullptr };

			for (size_t index = 0u; index < modelCount; ++index)
				tempModelBundle[index] = std::static_pointer_cast<Model>(models[index]);

			const std::vector<size_t> modelIndices
				= m_modelBuffers.AddMultiple(std::move(tempModelBundle));

			auto dvThis = static_cast<Derived*>(this);

			ModelBundleType modelBundleObj{};

			{
				std::vector<std::uint32_t> modelIndicesU32(std::size(modelIndices), 0u);
				for (size_t index = 0u; index < std::size(modelIndices); ++index)
					modelIndicesU32[index] = static_cast<std::uint32_t>(modelIndices[index]);

				dvThis->ConfigureModelBundle(
					modelBundleObj, std::move(modelIndicesU32), std::move(modelBundle), tempBuffer
				);
			}

			const std::uint32_t psoIndex = GetPSOIndex(pixelShader);

			modelBundleObj.SetPSOIndex(psoIndex);

			const auto bundleID = modelBundleObj.GetID();

			AddModelBundle(std::move(modelBundleObj));

			m_oldBufferCopyNecessary = true;

			return bundleID;
		}

		return std::numeric_limits<std::uint32_t>::max();
	}

	void RemoveModelBundle(std::uint32_t bundleID) noexcept
	{
		auto result = GetModelBundle(bundleID);

		if (result != std::end(m_modelBundles))
		{
			const auto modelBundleIndex = static_cast<size_t>(
				std::distance(std::begin(m_modelBundles), result)
				);

			static_cast<Derived*>(this)->ConfigureModelRemove(modelBundleIndex);

			m_modelBundles.erase(result);
		}
	}

	void AddPSO(const ShaderName& pixelShader)
	{
		GetPSOIndex(pixelShader);
	}

	void ChangePSO(std::uint32_t bundleID, const ShaderName& pixelShader)
	{
		auto modelBundle = GetModelBundle(bundleID);

		if (modelBundle != std::end(m_modelBundles))
		{
			const std::uint32_t psoIndex = GetPSOIndex(pixelShader);

			modelBundle->SetPSOIndex(psoIndex);

			ModelBundleType modelBundleObj = std::move(*modelBundle);

			m_modelBundles.erase(modelBundle);

			AddModelBundle(std::move(modelBundleObj));
		}
	}

	[[nodiscard]]
	std::uint32_t AddMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		TemporaryDataBufferGPU& tempBuffer
	) {
		MeshManager meshManager{};

		static_cast<Derived*>(this)->ConfigureMeshBundle(
			std::move(meshBundle), stagingBufferMan, meshManager, tempBuffer
		);

		auto meshIndex           = m_meshBundles.Add(std::move(meshManager));

		m_oldBufferCopyNecessary = true;

		return static_cast<std::uint32_t>(meshIndex);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
	{
		static_cast<Derived*>(this)->ConfigureRemoveMesh(bundleIndex);

		// It is okay to use the non-clear function based RemoveElement, as I will be
		// moving the Buffers out as SharedBuffer.
		m_meshBundles.RemoveElement(bundleIndex);
	}

	void RecreateGraphicsPipelines()
	{
		for (auto& graphicsPipeline : m_graphicsPipelines)
			graphicsPipeline.Recreate(m_device, m_graphicsRootSignature, m_shaderPath);
	}

protected:
	[[nodiscard]]
	std::optional<std::uint32_t> TryToGetPSOIndex(const ShaderName& pixelShader) const noexcept
	{
		auto result = std::ranges::find_if(m_graphicsPipelines,
			[&pixelShader](const Pipeline& pipeline)
			{
				return pixelShader == pipeline.GetPixelShader();
			});

		if (result != std::end(m_graphicsPipelines))
			return static_cast<std::uint32_t>(std::distance(std::begin(m_graphicsPipelines), result));
		else
			return {};
	}

	// Adds a new PSO, if one can't be found.
	std::uint32_t GetPSOIndex(const ShaderName& pixelShader)
	{
		std::uint32_t psoIndex = 0u;
		auto oPSOIndex         = TryToGetPSOIndex(pixelShader);

		if (!oPSOIndex)
		{
			psoIndex          = static_cast<std::uint32_t>(std::size(m_graphicsPipelines));

			Pipeline pipeline{};

			pipeline.Create(m_device, m_graphicsRootSignature, m_shaderPath, pixelShader);

			m_graphicsPipelines.emplace_back(std::move(pipeline));
		}
		else
			psoIndex = oPSOIndex.value();

		return psoIndex;
	}

	void BindPipeline(
		const ModelBundleType& modelBundle, const D3DCommandList& graphicsCmdList,
		size_t& previousPSOIndex
	) const noexcept {
		// PSO is more costly to bind, so the modelBundles are added in a way so they are sorted
		// by their PSO indices. And we only bind a new PSO, if the previous one was different.
		const size_t modelPSOIndex = modelBundle.GetPSOIndex();

		if (modelPSOIndex != previousPSOIndex)
		{
			m_graphicsPipelines[modelPSOIndex].Bind(graphicsCmdList);

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
	ID3D12Device5*               m_device;
	MemoryManager*               m_memoryManager;
	ID3D12RootSignature*         m_graphicsRootSignature;
	std::wstring                 m_shaderPath;
	ModelBuffers                 m_modelBuffers;
	std::vector<Pipeline>        m_graphicsPipelines;
	ReusableVector<MeshManager>  m_meshBundles;
	std::vector<ModelBundleType> m_modelBundles;
	bool                         m_oldBufferCopyNecessary;

	// The pixel and Vertex data are on different sets. So both can be 0u.
	static constexpr size_t s_modelBuffersPixelSRVRegisterSlot    = 0u;
	static constexpr size_t s_modelBuffersGraphicsSRVRegisterSlot = 0u;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ other.m_memoryManager },
		m_graphicsRootSignature{ other.m_graphicsRootSignature },
		m_shaderPath{ std::move(other.m_shaderPath) },
		m_modelBuffers{ std::move(other.m_modelBuffers) },
		m_graphicsPipelines{ std::move(other.m_graphicsPipelines) },
		m_meshBundles{ std::move(other.m_meshBundles) },
		m_modelBundles{ std::move(other.m_modelBundles) },
		m_oldBufferCopyNecessary{ other.m_oldBufferCopyNecessary }
	{}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		m_device                 = other.m_device;
		m_memoryManager          = other.m_memoryManager;
		m_graphicsRootSignature  = other.m_graphicsRootSignature;
		m_shaderPath             = std::move(other.m_shaderPath);
		m_modelBuffers           = std::move(other.m_modelBuffers);
		m_graphicsPipelines      = std::move(other.m_graphicsPipelines);
		m_meshBundles            = std::move(other.m_meshBundles);
		m_modelBundles           = std::move(other.m_modelBundles);
		m_oldBufferCopyNecessary = other.m_oldBufferCopyNecessary;

		return *this;
	}
};

class ModelManagerVSIndividual : public
	ModelManager
	<
		ModelManagerVSIndividual,
		GraphicsPipelineIndividualDraw,
		MeshManagerVertexShader,
		ModelBundleVSIndividual
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndividual,
			GraphicsPipelineIndividualDraw,
			MeshManagerVertexShader,
			ModelBundleVSIndividual
		>;
	friend class ModelManagerVSIndividualTest;

public:
	ModelManagerVSIndividual(
		ID3D12Device5* device, MemoryManager* memoryManager, std::uint32_t frameCount
	);

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers,
		size_t vsRegisterSpace, size_t psRegisterSpace
	);
	void SetDescriptors(
		std::vector<D3DDescriptorManager>& descriptorManagers,
		size_t vsRegisterSpace, size_t psRegisterSpace
	);

	void Draw(const D3DCommandList& graphicsList) const noexcept;

	void CopyOldBuffers(const D3DCommandList& copyList) noexcept;

private:
	void _setGraphicsConstantRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;

	void ConfigureModelBundle(
		ModelBundleVSIndividual& modelBundleObj,
		std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle,
		TemporaryDataBufferGPU& tempBuffer
	) const noexcept;

	void ConfigureModelRemove(size_t bundleIndex) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
	);

	void _updatePerFrame([[maybe_unused]] UINT64 frameIndex) const noexcept {}
	// To create compute shader pipelines.
	void ShaderPathSet() {}

private:
	UINT            m_constantsRootIndex;
	SharedBufferGPU m_vertexBuffer;
	SharedBufferGPU m_indexBuffer;

	// Vertex Shader ones
	static constexpr size_t s_constantDataCBVRegisterSlot = 0u;

public:
	ModelManagerVSIndividual(const ModelManagerVSIndividual&) = delete;
	ModelManagerVSIndividual& operator=(const ModelManagerVSIndividual&) = delete;

	ModelManagerVSIndividual(ModelManagerVSIndividual&& other) noexcept
		: ModelManager{ std::move(other) },
		m_constantsRootIndex{ other.m_constantsRootIndex },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) }
	{}
	ModelManagerVSIndividual& operator=(ModelManagerVSIndividual&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_constantsRootIndex  = other.m_constantsRootIndex;
		m_vertexBuffer        = std::move(other.m_vertexBuffer);
		m_indexBuffer         = std::move(other.m_indexBuffer);

		return *this;
	}
};

class ModelManagerVSIndirect : public
	ModelManager
	<
		ModelManagerVSIndirect,
		GraphicsPipelineIndirectDraw,
		MeshManagerVertexShader,
		ModelBundleVSIndirect
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndirect,
			GraphicsPipelineIndirectDraw,
			MeshManagerVertexShader,
			ModelBundleVSIndirect
		>;
	friend class ModelManagerVSIndirectTest;

	struct ConstantData
	{
		std::uint32_t modelCount;
	};

public:
	ModelManagerVSIndirect(
		ID3D12Device5* device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
		std::uint32_t frameCount
	);

	void ResetCounterBuffer(const D3DCommandList& computeList, size_t frameIndex) const noexcept;

	void SetComputeConstantRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;

	void SetComputeRootSignature(ID3D12RootSignature* rootSignature) noexcept
	{
		m_computeRootSignature = rootSignature;
	}

	void CopyOldBuffers(const D3DCommandList& copyList) noexcept;

	void SetDescriptorLayoutVS(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace,
		size_t psRegisterSpace
	) const noexcept;
	void SetDescriptorsVS(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace,
		size_t psRegisterSpace
	) const;

	void SetDescriptorLayoutCS(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
	) const noexcept;

	void SetDescriptorsCSOfModels(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
	) const;
	void SetDescriptorsCSOfMeshes(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
	) const;

	void Draw(
		size_t frameIndex, const D3DCommandList& graphicsList, ID3D12CommandSignature* commandSignature
	) const noexcept;
	void Dispatch(const D3DCommandList& computeList) const noexcept;

	[[nodiscard]]
	UINT GetConstantsVSRootIndex() const noexcept { return m_constantsVSRootIndex; }

private:
	void _setGraphicsConstantRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;

	void ConfigureModelBundle(
		ModelBundleVSIndirect& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle,
		TemporaryDataBufferGPU& tempBuffer
	);

	void ConfigureModelRemove(size_t bundleIndex) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
	);

	void _updatePerFrame(UINT64 frameIndex) const noexcept;

	// To create compute shader pipelines.
	void ShaderPathSet();

	void UpdateDispatchX() noexcept;
	void UpdateCounterResetValues();

	[[nodiscard]]
	static consteval UINT GetConstantCount() noexcept
	{
		return static_cast<UINT>(sizeof(ConstantData) / sizeof(UINT));
	}

private:
	StagingBufferManager*                 m_stagingBufferMan;
	std::vector<SharedBufferCPU>          m_argumentInputBuffers;
	std::vector<SharedBufferGPUWriteOnly> m_argumentOutputBuffers;
	SharedBufferCPU                       m_cullingDataBuffer;
	std::vector<SharedBufferGPUWriteOnly> m_counterBuffers;
	Buffer                                m_counterResetBuffer;
	MultiInstanceCPUBuffer<std::uint32_t> m_meshBundleIndexBuffer;
	SharedBufferGPU                       m_vertexBuffer;
	SharedBufferGPU                       m_indexBuffer;
	SharedBufferGPU                       m_perModelDataBuffer;
	SharedBufferGPU                       m_perMeshDataBuffer;
	SharedBufferGPU                       m_perMeshBundleDataBuffer;
	ID3D12RootSignature*                  m_computeRootSignature;
	ComputePipeline                       m_computePipeline;
	UINT                                  m_dispatchXCount;
	std::uint32_t                         m_argumentCount;
	UINT                                  m_constantsVSRootIndex;
	UINT                                  m_constantsCSRootIndex;

	// These CS models will have the data to be uploaded and the dispatching will be done on the Manager.
	std::vector<ModelBundleCSIndirect>    m_modelBundlesCS;

	// Vertex Shader ones
	// CBV
	static constexpr size_t s_constantDataVSCBVRegisterSlot = 0u;

	// Compute Shader ones
	// CBV
	static constexpr size_t s_constantDataCSCBVRegisterSlot      = 0u;
	// SRV
	static constexpr size_t s_modelBuffersCSSRVRegisterSlot      = 0u;
	static constexpr size_t s_argumentInputBufferSRVRegisterSlot = 1u;
	static constexpr size_t s_cullingDataBufferSRVRegisterSlot   = 2u;
	static constexpr size_t s_perModelDataSRVRegisterSlot        = 3u;
	static constexpr size_t s_perMeshDataSRVRegisterSlot         = 4u;
	static constexpr size_t s_perMeshBundleDataSRVRegisterSlot   = 5u;
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
		m_stagingBufferMan{ other.m_stagingBufferMan },
		m_argumentInputBuffers{ std::move(other.m_argumentInputBuffers) },
		m_argumentOutputBuffers{ std::move(other.m_argumentOutputBuffers) },
		m_cullingDataBuffer{ std::move(other.m_cullingDataBuffer) },
		m_counterBuffers{ std::move(other.m_counterBuffers) },
		m_counterResetBuffer{ std::move(other.m_counterResetBuffer) },
		m_meshBundleIndexBuffer{ std::move(other.m_meshBundleIndexBuffer) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) },
		m_perModelDataBuffer{ std::move(other.m_perModelDataBuffer) },
		m_perMeshDataBuffer{ std::move(other.m_perMeshDataBuffer) },
		m_perMeshBundleDataBuffer{ std::move(other.m_perMeshBundleDataBuffer) },
		m_computeRootSignature{ other.m_computeRootSignature },
		m_computePipeline{ std::move(other.m_computePipeline) },
		m_dispatchXCount{ other.m_dispatchXCount },
		m_argumentCount{ other.m_argumentCount },
		m_constantsVSRootIndex{ other.m_constantsVSRootIndex },
		m_constantsCSRootIndex{ other.m_constantsCSRootIndex },
		m_modelBundlesCS{ std::move(other.m_modelBundlesCS) }
	{}
	ModelManagerVSIndirect& operator=(ModelManagerVSIndirect&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_stagingBufferMan        = other.m_stagingBufferMan;
		m_argumentInputBuffers    = std::move(other.m_argumentInputBuffers);
		m_argumentOutputBuffers   = std::move(other.m_argumentOutputBuffers);
		m_cullingDataBuffer       = std::move(other.m_cullingDataBuffer);
		m_counterBuffers          = std::move(other.m_counterBuffers);
		m_counterResetBuffer      = std::move(other.m_counterResetBuffer);
		m_meshBundleIndexBuffer   = std::move(other.m_meshBundleIndexBuffer);
		m_vertexBuffer            = std::move(other.m_vertexBuffer);
		m_indexBuffer             = std::move(other.m_indexBuffer);
		m_perModelDataBuffer      = std::move(other.m_perModelDataBuffer);
		m_perMeshDataBuffer       = std::move(other.m_perMeshDataBuffer);
		m_perMeshBundleDataBuffer = std::move(other.m_perMeshBundleDataBuffer);
		m_computeRootSignature    = other.m_computeRootSignature;
		m_computePipeline         = std::move(other.m_computePipeline);
		m_dispatchXCount          = other.m_dispatchXCount;
		m_argumentCount           = other.m_argumentCount;
		m_constantsVSRootIndex    = other.m_constantsVSRootIndex;
		m_constantsCSRootIndex    = other.m_constantsCSRootIndex;
		m_modelBundlesCS          = std::move(other.m_modelBundlesCS);

		return *this;
	}
};

class ModelManagerMS : public
	ModelManager
	<
		ModelManagerMS,
		GraphicsPipelineMeshShader,
		MeshManagerMeshShader,
		ModelBundleMSIndividual
	>
{
	friend class ModelManager
		<
			ModelManagerMS,
			GraphicsPipelineMeshShader,
			MeshManagerMeshShader,
			ModelBundleMSIndividual
		>;
	friend class ModelManagerMSTest;

public:
	ModelManagerMS(
		ID3D12Device5* device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
		std::uint32_t frameCount
	);

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace,
		size_t psRegisterSpace
	) const noexcept;

	// Should be called after a new Mesh has been added.
	void SetDescriptorsOfMeshes(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
	) const;
	// Should be called after a new Model has been added.
	void SetDescriptorsOfModels(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace,
		size_t psRegisterSpace
	) const;

	void CopyOldBuffers(const D3DCommandList& copyList) noexcept;

	void Draw(const D3DCommandList& graphicsList) const noexcept;

private:
	void _setGraphicsConstantRootIndex(
		const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
	) noexcept;
	void ConfigureModelBundle(
		ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle, TemporaryDataBufferGPU& tempBuffer
	);

	void ConfigureModelRemove(size_t bundleIndex) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		MeshManagerMeshShader& meshManager, TemporaryDataBufferGPU& tempBuffer
	);

	void _updatePerFrame([[maybe_unused]] UINT64 frameIndex) const noexcept {}
	// To create compute shader pipelines.
	void ShaderPathSet() {}

private:
	UINT                  m_constantsRootIndex;
	StagingBufferManager* m_stagingBufferMan;
	SharedBufferGPU       m_perMeshletBuffer;
	SharedBufferGPU       m_vertexBuffer;
	SharedBufferGPU       m_vertexIndicesBuffer;
	SharedBufferGPU       m_primIndicesBuffer;

	// CBV
	static constexpr size_t s_constantDataCBVRegisterSlot        = 0u;
	// SRV
	static constexpr size_t s_perMeshletBufferSRVRegisterSlot    = 1u;
	static constexpr size_t s_vertexBufferSRVRegisterSlot        = 2u;
	static constexpr size_t s_vertexIndicesBufferSRVRegisterSlot = 3u;
	static constexpr size_t s_primIndicesBufferSRVRegisterSlot   = 4u;

public:
	ModelManagerMS(const ModelManagerMS&) = delete;
	ModelManagerMS& operator=(const ModelManagerMS&) = delete;

	ModelManagerMS(ModelManagerMS&& other) noexcept
		: ModelManager{ std::move(other) },
		m_constantsRootIndex{ other.m_constantsRootIndex },
		m_stagingBufferMan{ other.m_stagingBufferMan },
		m_perMeshletBuffer{ std::move(other.m_perMeshletBuffer) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_vertexIndicesBuffer{ std::move(other.m_vertexIndicesBuffer) },
		m_primIndicesBuffer{ std::move(other.m_primIndicesBuffer) }
	{}
	ModelManagerMS& operator=(ModelManagerMS&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_constantsRootIndex  = other.m_constantsRootIndex;
		m_stagingBufferMan    = other.m_stagingBufferMan;
		m_perMeshletBuffer    = std::move(other.m_perMeshletBuffer);
		m_vertexBuffer        = std::move(other.m_vertexBuffer);
		m_vertexIndicesBuffer = std::move(other.m_vertexIndicesBuffer);
		m_primIndicesBuffer   = std::move(other.m_primIndicesBuffer);

		return *this;
	}
};
#endif
