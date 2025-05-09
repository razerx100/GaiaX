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

	static const MeshBundleTemporaryData meshBundleData
	{
		.vertices       = { Vertex{} },
		.indices        = { 0u, 1u, 2u },
		.primIndices    = { 0u },
		.meshletDetails = { MeshletDetails{}, MeshletDetails{} },
		.bundleDetails = MeshBundleTemporaryDetails
		{
			.meshTemporaryDetailsVS = { MeshTemporaryDetailsVS{} },
			.meshTemporaryDetailsMS = { MeshTemporaryDetailsMS{} }
		}
	};
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

static void RemoveModelBundle(
	ModelContainer& modelContainer, const std::shared_ptr<ModelBundle>& modelBundle
) noexcept {
	modelContainer.RemoveModels(modelBundle->GetIndicesInContainer());
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

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

	auto modelContainer = std::make_shared<ModelContainer>();

	ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

	modelBuffers.SetModelContainer(modelContainer);

	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndividualMesh.RemoveMeshBundle(0u);
	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndividualMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(*modelContainer, vsIndividual.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 0u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		std::uint32_t index = vsIndividual.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

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

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

	auto modelContainer = std::make_shared<ModelContainer>();

	ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

	modelBuffers.SetModelContainer(modelContainer);

	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndirectMesh.RemoveMeshBundle(0u);
	{
		MeshBundleTemporaryData meshVS = Constants::meshBundleData;

		std::uint32_t index = vsIndirectMesh.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(*modelContainer, vsIndirect.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 0u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 3u) << "Index isn't 3.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 4u) << "Index isn't 4.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();
		auto pipeline    = PipelineModelBundle{};
		auto pipeline1   = PipelineModelBundle{};
		auto pipeline2   = PipelineModelBundle{};

		modelBundle->SetModelContainer(modelContainer);

		pipeline.SetPipelineIndex(1u);
		pipeline1.SetPipelineIndex(2u);
		pipeline2.SetPipelineIndex(3u);

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));
		modelBundle->AddPipeline(std::move(pipeline2));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 3u);

		for (size_t index = 0u; index < 3u; ++index)
			modelBundle->AddModel(Model{}, 2u);

		std::shared_ptr<ModelBundle> modelBundle1 = modelBundle;

		std::uint32_t index = vsIndirect.AddModelBundle(std::move(modelBundle1));

		modelBuffers.ExtendModelBuffers();

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

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

	auto modelContainer = std::make_shared<ModelContainer>();

	ModelBuffers modelBuffers{ device, &memoryManager, Constants::frameCount };

	modelBuffers.SetModelContainer(modelContainer);

	{
		MeshBundleTemporaryData meshMS = Constants::meshBundleData;

		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		MeshBundleTemporaryData meshMS = Constants::meshBundleData;

		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	managerMSMesh.RemoveMeshBundle(0u);
	{
		MeshBundleTemporaryData meshMS = Constants::meshBundleData;

		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		MeshBundleTemporaryData meshMS = Constants::meshBundleData;

		std::uint32_t index = managerMSMesh.AddMeshBundle(
			std::move(meshMS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2u";
	}

	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 0u);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	::RemoveModelBundle(*modelContainer, managerMS.RemoveModelBundle(1u));
	{
		auto modelBundle = std::make_shared<ModelBundle>();

		modelBundle->SetModelContainer(modelContainer);

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 0u);
		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundle = std::make_shared<ModelBundle>();
		auto pipeline    = PipelineModelBundle{};
		auto pipeline1   = PipelineModelBundle{};
		auto pipeline2   = PipelineModelBundle{};

		modelBundle->SetModelContainer(modelContainer);

		pipeline.SetPipelineIndex(1u);
		pipeline1.SetPipelineIndex(2u);
		pipeline2.SetPipelineIndex(3u);

		modelBundle->AddPipeline(std::move(pipeline));
		modelBundle->AddPipeline(std::move(pipeline1));
		modelBundle->AddPipeline(std::move(pipeline2));

		for (size_t index = 0u; index < 7u; ++index)
			modelBundle->AddModel(Model{}, 1u);

		for (size_t index = 0u; index < 5u; ++index)
			modelBundle->AddModel(Model{}, 3u);

		for (size_t index = 0u; index < 3u; ++index)
			modelBundle->AddModel(Model{}, 2u);

		std::shared_ptr<ModelBundle> modelBundle1 = modelBundle;

		std::uint32_t index = managerMS.AddModelBundle(std::move(modelBundle1));

		modelBuffers.ExtendModelBuffers();

		EXPECT_EQ(index, 3u) << "Index isn't 3.";

		modelBundle->ChangeModelPipeline(7u, 1u, 3u);
		managerMS.ReconfigureModels(index, 1u, 3u);
	}
}
