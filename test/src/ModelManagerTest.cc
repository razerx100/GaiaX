#include <gtest/gtest.h>
#include <memory>

#include <DeviceManager.hpp>
#include <ModelManager.hpp>
#include <D3DModelBuffer.hpp>
#include <StagingBufferManager.hpp>
#include <D3DDescriptorHeapManager.hpp>

namespace Constants
{
	constexpr std::uint32_t frameCount   = 2u;
	constexpr size_t descSetLayoutCount  = 2u;
	constexpr size_t vsRegisterSpace     = 0u;
	constexpr size_t csRegisterSpace     = 0u;
	constexpr size_t psRegisterSpace     = 1u;
	constexpr std::uint32_t meshBundleID = 0u;
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

class ModelDummy : public Model
{
	std::uint32_t m_psoIndex = 0u;
public:
	[[nodiscard]]
	DirectX::XMMATRIX GetModelMatrix() const noexcept override { return {}; }
	[[nodiscard]]
	DirectX::XMFLOAT3 GetModelOffset() const noexcept override { return {}; }
	[[nodiscard]]
	std::uint32_t GetMaterialIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	std::uint32_t GetMeshIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	std::uint32_t GetDiffuseIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetDiffuseUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	std::uint32_t GetSpecularIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetSpecularUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	float GetModelScale() const noexcept override { return 1.f; }
	[[nodiscard]]
	std::uint32_t GetPipelineIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	bool IsVisible() const noexcept override { return true; }

	void SetPipelineIndex(std::uint32_t psoIndex) noexcept { m_psoIndex = psoIndex; }
};

class ModelBundleDummy : public ModelBundle
{
	std::uint32_t                       m_meshBundleID = Constants::meshBundleID;
	std::vector<std::shared_ptr<Model>> m_models;

public:
	void AddModel(std::shared_ptr<Model> model) noexcept
	{
		m_models.emplace_back(std::move(model));
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept override { return m_meshBundleID; }
	[[nodiscard]]
	const std::vector<std::shared_ptr<Model>>& GetModels() const noexcept override
	{
		return m_models;
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
			models.emplace_back(std::make_shared<ModelDummy>());

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
			models.emplace_back(std::make_shared<ModelDummy>());

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
				models.emplace_back(std::make_shared<ModelDummy>());

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
				models.emplace_back(std::make_shared<ModelDummy>());

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
				models.emplace_back(std::make_shared<ModelDummy>());

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
				models.emplace_back(std::make_shared<ModelDummy>());

			std::vector<size_t> modelIndices = modelBuffers.AddMultiple(std::move(models));

			for (size_t index = 5u; index < std::size(modelIndices); ++index)
			{
				const size_t modelIndex = modelIndices.at(index);

				EXPECT_EQ(modelIndex, index) << "Model index doesn't match.";
			}
		}
	}
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

	TemporaryDataBufferGPU tempDataBuffer{};

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
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	modelBuffers.Remove(vsIndividual.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
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

	TemporaryDataBufferGPU tempDataBuffer{};

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
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	modelBuffers.Remove(vsIndirect.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 3u) << "Index isn't 3.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 4u) << "Index isn't 4.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(2u);

			modelBundle->AddModel(std::move(model));
		}
		for (size_t index = 0u; index < 5u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(1u);

			modelBundle->AddModel(std::move(model));
		}
		for (size_t index = 0u; index < 3u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(3u);

			modelBundle->AddModel(std::move(model));
		}

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 5u) << "Index isn't 5.";

		vsIndirect.ChangeModelPipeline(index, 7u, 1u, 3u);
		vsIndirect.ChangeModelPipeline(index, 4u, 2u, 1u);
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

	TemporaryDataBufferGPU tempDataBuffer{};

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
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto model       = std::make_shared<ModelDummy>();
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		modelBundle->AddModel(std::move(model));

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	modelBuffers.Remove(managerMS.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(std::make_shared<ModelDummy>());

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundleDummy>();

		for (size_t index = 0u; index < 7u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(2u);

			modelBundle->AddModel(std::move(model));
		}
		for (size_t index = 0u; index < 5u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(1u);

			modelBundle->AddModel(std::move(model));
		}
		for (size_t index = 0u; index < 3u; ++index)
		{
			auto model = std::make_shared<ModelDummy>();

			model->SetPipelineIndex(3u);

			modelBundle->AddModel(std::move(model));
		}

		std::vector<std::shared_ptr<Model>> models = modelBundle->GetModels();
		std::vector<std::uint32_t> modelIndices    = modelBuffers.AddMultipleRU32(std::move(models));

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle), std::move(modelIndices));
		EXPECT_EQ(index, 3u) << "Index isn't 3.";

		managerMS.ChangeModelPipeline(index, 7u, 1u, 3u);
	}
}
