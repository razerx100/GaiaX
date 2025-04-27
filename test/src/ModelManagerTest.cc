#include <gtest/gtest.h>
#include <memory>

#include <D3DDeviceManager.hpp>
#include <D3DModelManager.hpp>
#include <D3DModelBuffer.hpp>
#include <D3DStagingBufferManager.hpp>
#include <D3DDescriptorHeapManager.hpp>

using namespace Gaia;

namespace Constants
{
	constexpr std::uint32_t frameCount      = 2u;
	constexpr size_t descSetLayoutCount     = 2u;
	constexpr size_t vsRegisterSpace        = 0u;
	constexpr size_t csRegisterSpace        = 0u;
	constexpr size_t psRegisterSpace        = 1u;
	constexpr std::uint32_t meshBundleIndex = 0u;
}

class ModelManagerTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<DeviceManager> s_deviceManager;
};

void ModelManagerTest::SetUpTestSuite()
{
	s_deviceManager = std::make_unique<DeviceManager>();

	s_deviceManager->GetDebugLogger().AddCallbackType(DebugCallbackType::StandardError);
	s_deviceManager->Create(D3D_FEATURE_LEVEL_12_0);
}

void ModelManagerTest::TearDownTestSuite()
{
	s_deviceManager.reset();
}

class ModelBundleDummy
{
	std::shared_ptr<ModelBundle> m_modelBundle;

	using ModelContainer_t    = ModelBundle::ModelContainer_t;
	using PipelineContainer_t = ModelBundle::PipelineContainer_t;

public:
	ModelBundleDummy() : m_modelBundle{ std::make_shared<ModelBundle>() }
	{
		m_modelBundle->SetMeshBundleIndex(Constants::meshBundleIndex);
	}

	void AddModel(std::uint32_t pipelineIndex, std::shared_ptr<Model> model) noexcept
	{
		ModelContainer_t& models       = m_modelBundle->GetModels();
		PipelineContainer_t& pipelines = m_modelBundle->GetPipelineBundles();

		const auto modelIndex = static_cast<std::uint32_t>(std::size(models));

		models.emplace_back(std::move(model));

		pipelines[pipelineIndex]->AddModelIndex(modelIndex);
	}

	std::uint32_t AddPipeline(std::shared_ptr<PipelineModelBundle> pipeline) noexcept
	{
		PipelineContainer_t& pipelines = m_modelBundle->GetPipelineBundles();

		const auto pipelineIndex = static_cast<std::uint32_t>(std::size(pipelines));

		pipelines.emplace_back(std::move(pipeline));

		return pipelineIndex;
	}

	void ChangeModelPipeline(
		std::uint32_t modelIndexInBundle, std::uint32_t oldPipelineIndex,
		std::uint32_t newPipelineIndex
	) noexcept {
		size_t oldPipelineIndexInBundle = std::numeric_limits<size_t>::max();
		size_t newPipelineIndexInBundle = std::numeric_limits<size_t>::max();

		PipelineContainer_t& pipelines = m_modelBundle->GetPipelineBundles();

		const size_t pipelineCount = std::size(pipelines);

		for (size_t index = 0u; index < pipelineCount; ++index)
		{
			const size_t currentPipelineIndex = pipelines[index]->GetPipelineIndex();

			if (currentPipelineIndex == oldPipelineIndex)
				oldPipelineIndexInBundle = index;

			if (currentPipelineIndex == newPipelineIndex)
				newPipelineIndexInBundle = index;

			const bool bothFound = oldPipelineIndexInBundle != std::numeric_limits<size_t>::max()
				&& newPipelineIndexInBundle != std::numeric_limits<size_t>::max();

			if (bothFound)
				break;
		}

		pipelines[oldPipelineIndexInBundle]->RemoveModelIndex(modelIndexInBundle);
		pipelines[newPipelineIndexInBundle]->AddModelIndex(modelIndexInBundle);
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept
	{
		return m_modelBundle->GetMeshBundleIndex();
	}
	[[nodiscard]]
	auto&& GetModels(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(self.m_modelBundle->GetModels());
	}
	[[nodiscard]]
	auto&& GetPipelineBundles(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(self.m_modelBundle->GetPipelineBundles());
	}

	[[nodiscard]]
	std::shared_ptr<ModelBundle> GetModelBundle() const noexcept
	{
		return m_modelBundle;
	}
};

class MeshBundleTemporaryDummy : public MeshBundleTemporary
{
	std::vector<MeshletDetails> m_meshletDetails = { MeshletDetails{}, MeshletDetails{} };
	std::vector<Vertex>         m_vertices = { Vertex{} };
	std::vector<std::uint32_t>  m_vertexIndices = { 0u, 1u, 2u };
	std::vector<std::uint32_t>  m_primIndices = { 0u };
	MeshBundleTemporaryDetails  m_bundleDetails{ .meshTemporaryDetailsVS = { MeshTemporaryDetailsVS{} } };

public:
	void GenerateTemporaryData(bool) override {}

	// Vertex and Mesh
	[[nodiscard]]
	const std::vector<Vertex>& GetVertices() const noexcept override
	{
		return m_vertices;
	}
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetVertexIndices() const noexcept override
	{
		return m_vertexIndices;
	}
	[[nodiscard]]
	const MeshBundleTemporaryDetails& GetTemporaryBundleDetails() const noexcept override
	{
		return m_bundleDetails;
	}
	[[nodiscard]]
	MeshBundleTemporaryDetails&& GetTemporaryBundleDetails() noexcept override
	{
		return std::move(m_bundleDetails);
	}

	// Mesh only
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetPrimIndices() const noexcept override
	{
		return m_primIndices;
	}
	[[nodiscard]]
	const std::vector<MeshletDetails>& GetMeshletDetails() const noexcept override
	{
		return m_meshletDetails;
	}
};

TEST_F(ModelManagerTest, ModelBufferTest)
{
	ID3D12Device* device   = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	// Single Model Once.
	{
		ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

		std::vector<std::shared_ptr<Model>> models{};
		for (size_t index = 0u; index < 6u; ++index)
			models.emplace_back(std::make_shared<Model>());

		for (size_t index = 0u; index < std::size(models); ++index)
		{
			const size_t modelIndex = modelBuffers.Add(std::move(models.at(index)));

			EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
		}
	}

	// Single Model with delete.
	{
		ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

		std::vector<std::shared_ptr<Model>> models{};
		for (size_t index = 0u; index < 6u; ++index)
			models.emplace_back(std::make_shared<Model>());

		{
			const size_t modelIndex = modelBuffers.Add(std::move(models.at(0u)));

			modelBuffers.Remove(modelIndex);
		}

		for (size_t index = 1u; index < std::size(models); ++index)
		{
			const size_t modelIndex = modelBuffers.Add(std::move(models.at(index)));

			EXPECT_EQ(modelIndex, index - 1u) << "Model index doesn't match.";
		}
	}

	// Multiple Model Once.
	{
		ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

		{
			std::vector<std::shared_ptr<Model>> models{};
			for (size_t index = 0u; index < 6u; ++index)
				models.emplace_back(std::make_shared<Model>());

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

			for (size_t index = 0u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
		{
			std::vector<std::shared_ptr<Model>> models{};
			for (size_t index = 0u; index < 4u; ++index)
				models.emplace_back(std::make_shared<Model>());

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

			for (size_t index = 6u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
	}

	// Multiple Model with delete.
	{
		ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

		{
			std::vector<std::shared_ptr<Model>> models{};
			for (size_t index = 0u; index < 6u; ++index)
				models.emplace_back(std::make_shared<Model>());

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

			for (size_t index = 0u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
		modelBuffers.Remove(5u);
		{
			std::vector<std::shared_ptr<Model>> models{};
			for (size_t index = 0u; index < 5u; ++index)
				models.emplace_back(std::make_shared<Model>());

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

			for (size_t index = 5u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
	}
}

static void RemoveModelBundle(
	ModelBuffers& modelBuffer, const ModelBundle& modelBundle
) noexcept {
	const auto& models = modelBundle.GetModels();

	for (const auto& model : models)
		modelBuffer.Remove(model->GetModelIndexInBuffer());
}
TEST_F(ModelManagerTest, ModelManagerVSIndividualTest)
{
	ID3D12Device5* device  = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{ device, &memoryManager, &threadPool };

	ModelManagerVSIndividual vsIndividual{};
	MeshManagerVSIndividual vsIndividualMesh{ device, &memoryManager };

	std::vector<D3DDescriptorManager> descManagers{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
		descManagers.emplace_back(D3DDescriptorManager{ device, Constants::descSetLayoutCount });

	vsIndividual.SetDescriptorLayout(descManagers, Constants::vsRegisterSpace);

	for (auto& descManager : descManagers)
		descManager.CreateDescriptors();

	D3DRootSignature rootSignature{};

	if (!std::empty(descManagers))
	{
		D3DDescriptorManager& graphicsDescriptorManager = descManagers.front();

		vsIndividual.SetGraphicsConstantsRootIndex(
			graphicsDescriptorManager, Constants::vsRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(graphicsDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::UnboundArray
		);

		rootSignature.CreateSignature(device, rootSignatureDynamic);
	}

	ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

	{
		auto meshVS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshVS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndividualMesh.RemoveMeshBundle(0u);
	{
		auto meshVS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(modelBuffers, *vsIndividual.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndividual.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
}

TEST_F(ModelManagerTest, ModelManagerVSIndirectTest)
{
	ID3D12Device5* device  = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{ device, &memoryManager, &threadPool };

	ModelManagerVSIndirect vsIndirect{ device, &memoryManager, Constants::frameCount };

	MeshManagerVSIndirect vsIndirectMesh{ device, &memoryManager };

	std::vector<D3DDescriptorManager> descManagersVS{};
	std::vector<D3DDescriptorManager> descManagersCS{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
	{
		descManagersVS.emplace_back(D3DDescriptorManager{ device, Constants::descSetLayoutCount });
		descManagersCS.emplace_back(D3DDescriptorManager{ device, Constants::descSetLayoutCount });
	}

	vsIndirect.SetDescriptorLayoutVS(descManagersVS, Constants::vsRegisterSpace);
	vsIndirect.SetDescriptorLayoutCS(descManagersCS, Constants::csRegisterSpace);

	for (auto& descManager : descManagersVS)
		descManager.CreateDescriptors();
	for (auto& descManager : descManagersCS)
		descManager.CreateDescriptors();

	D3DRootSignature rootSignatureVS{};

	if (!std::empty(descManagersVS))
	{
		D3DDescriptorManager& graphicsDescriptorManager = descManagersVS.front();

		vsIndirect.SetGraphicsConstantsRootIndex(
			graphicsDescriptorManager, Constants::vsRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(graphicsDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::UnboundArray
		);

		rootSignatureVS.CreateSignature(device, rootSignatureDynamic);
	}

	D3DRootSignature rootSignatureCS{};

	if (!std::empty(descManagersCS))
	{
		D3DDescriptorManager& computeDescriptorManager = descManagersCS.front();

		vsIndirect.SetComputeConstantsRootIndex(
			computeDescriptorManager, Constants::csRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(computeDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.ComputeShader(), BindlessLevel::UnboundArray
		);

		rootSignatureVS.CreateSignature(device, rootSignatureDynamic);
	}

	ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

	{
		auto meshVS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshVS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndirectMesh.RemoveMeshBundle(0u);
	{
		auto meshVS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(modelBuffers, *vsIndirect.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 3u) << "Index isn't 3.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 4u) << "Index isn't 4.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();
		auto pipeline2   = std::make_shared<PipelineModelBundle>();

		pipeline->SetPipelineIndex(1u);
		pipeline1->SetPipelineIndex(2u);
		pipeline2->SetPipelineIndex(3u);

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));
		modelBundle->AddPipeline(std::move(pipeline2));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(1u, std::move(std::make_shared<Model>()));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::move(std::make_shared<Model>()));

		for (size_t index = 0u; index < 3u; ++index)
			modelBundle->AddModel(2u, std::move(std::make_shared<Model>()));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = vsIndirect.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 5u) << "Index isn't 5.";

		modelBundle->ChangeModelPipeline(7u, 1u, 3u);
		vsIndirect.ReconfigureModels(index, 1u, 3u);

		modelBundle->ChangeModelPipeline(5u, 2u, 1u);
		modelBundle->ChangeModelPipeline(4u, 2u, 1u);
		vsIndirect.ReconfigureModels(index, 2u, 1u);
	}
}

TEST_F(ModelManagerTest, ModelManagerMS)
{
	ID3D12Device5* device  = s_deviceManager->GetDevice();
	IDXGIAdapter3* adapter = s_deviceManager->GetAdapter();

	MemoryManager memoryManager{ adapter, device, 20_MB, 200_KB };

	ThreadPool threadPool{ 2u };

	StagingBufferManager stagingBufferManager{ device, &memoryManager, &threadPool };

	ModelManagerMS managerMS{};

	MeshManagerMS managerMSMesh{ device, &memoryManager };

	std::vector<D3DDescriptorManager> descManagers{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
		descManagers.emplace_back(D3DDescriptorManager{ device, Constants::descSetLayoutCount });

	managerMS.SetDescriptorLayout(descManagers, Constants::vsRegisterSpace);

	for (auto& descManager : descManagers)
		descManager.CreateDescriptors();

	D3DRootSignature rootSignature{};

	if (!std::empty(descManagers))
	{
		D3DDescriptorManager& graphicsDescriptorManager = descManagers.front();

		managerMS.SetGraphicsConstantsRootIndex(
			graphicsDescriptorManager, Constants::vsRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(graphicsDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.MeshShader(), BindlessLevel::UnboundArray
		);

		rootSignature.CreateSignature(device, rootSignatureDynamic);
	}

	ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

	{
		auto meshMS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshMS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	managerMSMesh.RemoveMeshBundle(0u);
	{
		auto meshMS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshMS         = std::make_unique<MeshBundleTemporaryDummy>();
		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2u";
	}

	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<Model>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto modelBundle = std::make_unique<ModelBundleDummy>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddModel(0u, std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(modelBuffers, *managerMS.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(0u, std::make_shared<Model>());
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(1u, std::make_shared<Model>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_unique<ModelBundleDummy>();
		auto pipeline    = std::make_shared<PipelineModelBundle>();
		auto pipeline1   = std::make_shared<PipelineModelBundle>();
		auto pipeline2   = std::make_shared<PipelineModelBundle>();

		pipeline->SetPipelineIndex(1u);
		pipeline1->SetPipelineIndex(2u);
		pipeline2->SetPipelineIndex(3u);

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));
		modelBundle->AddPipeline(std::move(pipeline2));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(1u, std::move(std::make_shared<Model>()));

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(0u, std::move(std::make_shared<Model>()));

		for (size_t index = 0u; index < 3u; ++index)
			modelBundle->AddModel(2u, std::move(std::make_shared<Model>()));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(
			std::move(models)
		);

		std::uint32_t index = managerMS.AddModelBundle(
			modelBundle->GetModelBundle(), modelIndices
		);
		EXPECT_EQ(index, 3u) << "Index isn't 3.";

		modelBundle->ChangeModelPipeline(7u, 1u, 3u);
		managerMS.ReconfigureModels(index, 1u, 3u);
	}
}
