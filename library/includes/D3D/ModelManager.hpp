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
#include <GraphicsPipelineVertexShader.hpp>
#include <GraphicsPipelineMeshShader.hpp>
#include <ComputePipeline.hpp>
#include <D3DModelBundle.hpp>
#include <D3DModelBuffer.hpp>

template<
	class Derived,
	class Pipeline,
	class MeshManager,
	class ModelBundleType
>
class ModelManager
{
public:
	ModelManager(ID3D12Device5* device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_graphicsRootSignature{ nullptr }, m_shaderPath{},
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
		ModelBuffers& modelBuffers, TemporaryDataBufferGPU& tempBuffer
	) {
		const std::vector<std::shared_ptr<Model>>& models = modelBundle->GetModels();

		if (!std::empty(models))
		{
			std::vector<std::shared_ptr<Model>> copyModels = models;

			std::vector<std::uint32_t> modelIndices        = modelBuffers.AddMultipleRU32(
				std::move(copyModels)
			);

			ModelBundleType modelBundleObj{};

			static_cast<Derived*>(this)->ConfigureModelBundle(
				modelBundleObj, std::move(modelIndices), std::move(modelBundle), tempBuffer
			);

			const std::uint32_t psoIndex = GetPSOIndex(pixelShader);

			modelBundleObj.SetPSOIndex(psoIndex);

			const std::uint32_t bundleID = modelBundleObj.GetID();

			AddModelBundle(std::move(modelBundleObj));

			m_oldBufferCopyNecessary = true;

			return bundleID;
		}

		return std::numeric_limits<std::uint32_t>::max();
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

		size_t meshIndex         = m_meshBundles.Add(std::move(meshManager));

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
		for (Pipeline& graphicsPipeline : m_graphicsPipelines)
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
		std::uint32_t psoIndex                 = 0u;
		std::optional<std::uint32_t> oPSOIndex = TryToGetPSOIndex(pixelShader);

		if (!oPSOIndex)
		{
			psoIndex = static_cast<std::uint32_t>(std::size(m_graphicsPipelines));

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
	std::vector<Pipeline>        m_graphicsPipelines;
	ReusableVector<MeshManager>  m_meshBundles;
	std::vector<ModelBundleType> m_modelBundles;
	bool                         m_oldBufferCopyNecessary;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ other.m_memoryManager },
		m_graphicsRootSignature{ other.m_graphicsRootSignature },
		m_shaderPath{ std::move(other.m_shaderPath) },
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
		D3DMeshBundleVS,
		ModelBundleVSIndividual
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndividual,
			GraphicsPipelineIndividualDraw,
			D3DMeshBundleVS,
			ModelBundleVSIndividual
		>;
	friend class ModelManagerVSIndividualTest;

public:
	ModelManagerVSIndividual(ID3D12Device5* device, MemoryManager* memoryManager);

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
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

	void ConfigureModelBundleRemove(size_t bundleIndex, ModelBuffers& modelBuffers) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		D3DMeshBundleVS& d3dMeshBundle, TemporaryDataBufferGPU& tempBuffer
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
		D3DMeshBundleVS,
		ModelBundleVSIndirect
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndirect,
			GraphicsPipelineIndirectDraw,
			D3DMeshBundleVS,
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
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
	) const noexcept;

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

	void ConfigureModelBundleRemove(size_t bundleIndex, ModelBuffers& modelBuffers) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		D3DMeshBundleVS& d3dMeshBundle, TemporaryDataBufferGPU& tempBuffer
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
		D3DMeshBundleMS,
		ModelBundleMSIndividual
	>
{
	friend class ModelManager
		<
			ModelManagerMS,
			GraphicsPipelineMeshShader,
			D3DMeshBundleMS,
			ModelBundleMSIndividual
		>;
	friend class ModelManagerMSTest;

public:
	ModelManagerMS(
		ID3D12Device5* device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan
	);

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
	) const noexcept;

	// Should be called after a new Mesh has been added.
	void SetDescriptorsOfMeshes(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
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

	void ConfigureModelBundleRemove(size_t bundleIndex, ModelBuffers& modelBuffers) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		D3DMeshBundleMS& d3dMeshBundle, TemporaryDataBufferGPU& tempBuffer
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
