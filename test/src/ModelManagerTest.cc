#include <gtest/gtest.h>
#include <memory>

#include <DeviceManager.hpp>
#include <ModelManager.hpp>
#include <StagingBufferManager.hpp>
#include <D3DDescriptorHeapManager.hpp>

namespace Constants
{
	constexpr std::uint32_t frameCount         = 2u;
	constexpr std::uint32_t descSetLayoutCount = 2u;
	constexpr std::uint32_t vsSetLayoutIndex   = 0u;
	constexpr std::uint32_t psSetLayoutIndex   = 1u;
	constexpr std::uint32_t meshID             = 0u;
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
public:
	[[nodiscard]]
	DirectX::XMMATRIX GetModelMatrix() const noexcept override { return {}; }
	[[nodiscard]]
	DirectX::XMFLOAT3 GetModelOffset() const noexcept override { return {}; }
	[[nodiscard]]
	std::uint32_t GetMaterialIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	std::uint32_t GetDiffuseIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetDiffuseUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	std::uint32_t GetSpecularIndex() const noexcept { return 0u; }
	[[nodiscard]]
	UVInfo GetSpecularUVInfo() const noexcept { return UVInfo{}; }
};

class ModelDummyVS : public ModelVS
{
	MeshDetailsVS m_details = {};

public:
	[[nodiscard]]
	DirectX::XMMATRIX GetModelMatrix() const noexcept override { return {}; }
	[[nodiscard]]
	DirectX::XMFLOAT3 GetModelOffset() const noexcept override { return {}; }
	[[nodiscard]]
	std::uint32_t GetMaterialIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	MeshDetailsVS GetMeshDetailsVS() const noexcept override
	{
		return m_details;
	}
	[[nodiscard]]
	std::uint32_t GetDiffuseIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetDiffuseUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	std::uint32_t GetSpecularIndex() const noexcept { return 0u; }
	[[nodiscard]]
	UVInfo GetSpecularUVInfo() const noexcept { return UVInfo{}; }
};

class ModelBundleDummyVS : public ModelBundleVS
{
	std::uint32_t                         m_meshID = Constants::meshID;
	std::vector<std::shared_ptr<ModelVS>> m_models;

public:
	void AddModel(std::shared_ptr<ModelVS> model) noexcept
	{
		m_models.emplace_back(std::move(model));
	}

	[[nodiscard]]
	std::uint32_t GetMeshIndex() const noexcept override { return m_meshID; }
	[[nodiscard]]
	const std::vector<std::shared_ptr<ModelVS>>& GetModels() const noexcept override
	{
		return m_models;
	}
};

class ModelBundleDummyMS : public ModelBundleMS
{
	std::uint32_t                         m_meshID = Constants::meshID;
	std::vector<std::shared_ptr<ModelMS>> m_models;

public:
	void AddModel(std::shared_ptr<ModelMS> model) noexcept
	{
		m_models.emplace_back(std::move(model));
	}

	[[nodiscard]]
	std::uint32_t GetMeshIndex() const noexcept override { return m_meshID; }
	[[nodiscard]]
	const std::vector<std::shared_ptr<ModelMS>>& GetModels() const noexcept override
	{
		return m_models;
	}
};

class ModelDummyMS : public ModelMS
{
	MeshDetailsMS m_details = {};

public:
	ModelDummyMS() : m_details{} {}

	[[nodiscard]]
	DirectX::XMMATRIX GetModelMatrix() const noexcept override { return {}; }
	[[nodiscard]]
	DirectX::XMFLOAT3 GetModelOffset() const noexcept override { return {}; }
	[[nodiscard]]
	std::uint32_t GetMaterialIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	MeshDetailsMS GetMeshDetailsMS() const noexcept override
	{
		return m_details;
	}
	[[nodiscard]]
	std::uint32_t GetDiffuseIndex() const noexcept override { return 0u; }
	[[nodiscard]]
	UVInfo GetDiffuseUVInfo() const noexcept override { return UVInfo{}; }
	[[nodiscard]]
	std::uint32_t GetSpecularIndex() const noexcept { return 0u; }
	[[nodiscard]]
	UVInfo GetSpecularUVInfo() const noexcept { return UVInfo{}; }
};

class MeshDummyVS : public MeshBundleVS
{
	std::vector<MeshBound>     m_bounds   = { MeshBound{} };
	std::vector<Vertex>        m_vertices = { Vertex{} };
	std::vector<std::uint32_t> m_indices  = { 0u, 1u, 2u };

public:
	[[nodiscard]]
	const std::vector<MeshBound>& GetBounds() const noexcept override
	{
		return m_bounds;
	}
	[[nodiscard]]
	const std::vector<Vertex>& GetVertices() const noexcept override
	{
		return m_vertices;
	}

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetIndices() const noexcept override
	{
		return m_indices;
	}

	void CleanUpVertices() noexcept
	{
		m_vertices = std::vector<Vertex>{};
	}
};

class MeshDummyMS : public MeshBundleMS
{
	std::vector<Meshlet>       m_meshlets      = { Meshlet{}, Meshlet{} };
	std::vector<MeshBound>     m_bounds        = { MeshBound{} };
	std::vector<Vertex>        m_vertices      = { Vertex{} };
	std::vector<std::uint32_t> m_vertexIndices = { 0u, 1u, 2u };
	std::vector<std::uint32_t> m_primIndices   = { 0u };

public:
	[[nodiscard]]
	const std::vector<MeshBound>& GetBounds() const noexcept override
	{
		return m_bounds;
	}
	[[nodiscard]]
	const std::vector<Vertex>& GetVertices() const noexcept override
	{
		return m_vertices;
	}

	void CleanUpVertices() noexcept
	{
		m_vertices = std::vector<Vertex>{};
	}

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetVertexIndices() const noexcept override
	{
		return m_vertexIndices;
	}
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetPrimIndices() const noexcept override
	{
		return m_primIndices;
	}
	[[nodiscard]]
	const std::vector<Meshlet>& GetMeshlets() const noexcept override
	{
		return m_meshlets;
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

	ModelManagerVSIndividual vsIndividual{ device, &memoryManager, Constants::frameCount };

	std::vector<D3DDescriptorManager> descBuffers{};

	for (size_t _ = 0u; _ < Constants::frameCount; ++_)
		descBuffers.emplace_back(
			D3DDescriptorManager{ device, Constants::descSetLayoutCount }
		);

	vsIndividual.SetDescriptorLayout(
		descBuffers, Constants::vsSetLayoutIndex, Constants::psSetLayoutIndex
	);

	for (auto& descBuffer : descBuffers)
		descBuffer.CreateDescriptors();

	vsIndividual.CreateRootSignature(descBuffers.front(), Constants::vsSetLayoutIndex);

	TemporaryDataBufferGPU tempDataBuffer{};

	{
		auto meshVS         = std::make_unique<MeshDummyVS>();
		std::uint32_t index = vsIndividual.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}
	{
		auto meshVS         = std::make_unique<MeshDummyVS>();
		std::uint32_t index = vsIndividual.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1u";
	}
	vsIndividual.RemoveMeshBundle(0u);
	{
		auto meshVS         = std::make_unique<MeshDummyVS>();
		std::uint32_t index = vsIndividual.AddMeshBundle(
			std::move(meshVS), stagingBufferManager, tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0u";
	}

	{
		auto modelVS       = std::make_shared<ModelDummyVS>();
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		modelBundleVS->AddModel(std::move(modelVS));

		std::uint32_t index = vsIndividual.AddModelBundle(
			std::move(modelBundleVS), L"", tempDataBuffer
		);
		EXPECT_EQ(index, 0u) << "Index isn't 0.";
	}
	{
		auto modelVS       = std::make_shared<ModelDummyVS>();
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		modelBundleVS->AddModel(std::move(modelVS));

		std::uint32_t index = vsIndividual.AddModelBundle(
			std::move(modelBundleVS), L"", tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
	{
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		for (size_t index = 0u; index < 5u; ++index)
			modelBundleVS->AddModel(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndividual.AddModelBundle(
			std::move(modelBundleVS), L"H", tempDataBuffer
		);
		EXPECT_EQ(index, 2u) << "Index isn't 2.";
	}
	vsIndividual.RemoveModelBundle(1u);
	{
		auto modelBundleVS = std::make_shared<ModelBundleDummyVS>();

		for (size_t index = 0u; index < 7u; ++index)
			modelBundleVS->AddModel(std::make_shared<ModelDummyVS>());

		std::uint32_t index = vsIndividual.AddModelBundle(
			std::move(modelBundleVS), L"H", tempDataBuffer
		);
		EXPECT_EQ(index, 1u) << "Index isn't 1.";
	}
}
