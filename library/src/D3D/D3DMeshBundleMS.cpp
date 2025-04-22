#include <D3DMeshBundleMS.hpp>
#include <VectorToSharedPtr.hpp>

namespace Gaia
{
D3DMeshBundleMS::D3DMeshBundleMS()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u },
	m_vertexIndicesBufferSharedData{ nullptr, 0u, 0u }, m_primIndicesBufferSharedData{ nullptr, 0u, 0u },
	m_perMeshletBufferSharedData{ nullptr, 0u, 0u }, m_perMeshSharedData{ nullptr, 0u, 0u },
	m_perMeshBundleSharedData{ nullptr, 0u, 0u }, m_meshBundleDetails{ 0u, 0u, 0u, 0u },
	m_bundleDetails{}
{}

void D3DMeshBundleMS::_setMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	const std::vector<Vertex>& vertices  = meshBundle->GetVertices();

	auto ConfigureBuffer = []<typename T>
		(
			const std::vector<T>& elements, StagingBufferManager& stagingBufferMan,
			SharedBufferGPU& sharedBuffer, SharedBufferData& sharedData,
			std::uint32_t& detailOffset, Callisto::TemporaryDataBufferGPU& tempBuffer
		)
	{
		constexpr auto stride = static_cast<UINT64>(sizeof(T));
		const auto bufferSize = static_cast<UINT64>(stride * std::size(elements));

		sharedData   = sharedBuffer.AllocateAndGetSharedData(bufferSize, tempBuffer);
		detailOffset = static_cast<std::uint32_t>(sharedData.offset / stride);

		std::shared_ptr<std::uint8_t[]> tempDataBuffer = Callisto::CopyVectorToSharedPtr(elements);

		stagingBufferMan.AddBuffer(
			std::move(tempDataBuffer), bufferSize, sharedData.bufferData, sharedData.offset,
			tempBuffer
		);
	};

	const std::vector<std::uint32_t>& vertexIndices   = meshBundle->GetVertexIndices();
	const std::vector<std::uint32_t>& primIndices     = meshBundle->GetPrimIndices();
	const std::vector<MeshletDetails>& meshletDetails = meshBundle->GetMeshletDetails();

	ConfigureBuffer(
		vertices, stagingBufferMan, vertexSharedBuffer, m_vertexBufferSharedData,
		m_meshBundleDetails.vertexOffset, tempBuffer
	);
	ConfigureBuffer(
		vertexIndices, stagingBufferMan, vertexIndicesSharedBuffer, m_vertexIndicesBufferSharedData,
		m_meshBundleDetails.vertexIndicesOffset, tempBuffer
	);
	ConfigureBuffer(
		primIndices, stagingBufferMan, primIndicesSharedBuffer, m_primIndicesBufferSharedData,
		m_meshBundleDetails.primIndicesOffset, tempBuffer
	);
	ConfigureBuffer(
		meshletDetails, stagingBufferMan, perMeshletSharedBuffer, m_perMeshletBufferSharedData,
		m_meshBundleDetails.meshletOffset, tempBuffer
	);

	m_bundleDetails = std::move(meshBundle->GetTemporaryBundleDetails());
}

void D3DMeshBundleMS::SetMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	// Init the temp data.
	meshBundle->GenerateTemporaryData(true);

	_setMeshBundle(
		std::move(meshBundle),
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		perMeshletSharedBuffer, tempBuffer
	);
}

void D3DMeshBundleMS::SetMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
	SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	constexpr auto perMeshDataStride = sizeof(AxisAlignedBoundingBox);

	// Init the temp data.
	meshBundle->GenerateTemporaryData(true);

	// Need this or else the overload which returns the R value ref will be called.
	const MeshBundleTemporary& meshBundleR = *meshBundle;
	const std::vector<MeshTemporaryDetailsMS>& meshDetailsMS
		= meshBundleR.GetTemporaryBundleDetails().meshTemporaryDetailsMS;

	const size_t meshCount     = std::size(meshDetailsMS);
	const auto perMeshDataSize = static_cast<UINT64>(perMeshDataStride * meshCount);

	m_perMeshSharedData = perMeshSharedBuffer.AllocateAndGetSharedData(
		perMeshDataSize, tempBuffer
	);

	auto perMeshBufferData = std::make_shared<std::uint8_t[]>(perMeshDataSize);

	{
		size_t perMeshOffset             = 0u;
		std::uint8_t* perMeshBufferStart = perMeshBufferData.get();

		for (const MeshTemporaryDetailsMS& meshDetail : meshDetailsMS)
		{
			memcpy(perMeshBufferStart + perMeshOffset, &meshDetail.aabb, perMeshDataStride);

			perMeshOffset += perMeshDataStride;
		}
	}

	// Mesh Bundle Data
	constexpr size_t perMeshBundleDataSize = sizeof(PerMeshBundleData);

	auto perBundleData        = std::make_shared<std::uint8_t[]>(perMeshBundleDataSize);

	m_perMeshBundleSharedData = perMeshBundleSharedBuffer.AllocateAndGetSharedData(
		perMeshBundleDataSize, tempBuffer
	);

	{
		PerMeshBundleData bundleData
		{
			.meshOffset = static_cast<std::uint32_t>(m_perMeshSharedData.offset / perMeshDataStride)
		};

		memcpy(perBundleData.get(), &bundleData, perMeshBundleDataSize);
	}

	stagingBufferMan.AddBuffer(
		std::move(perBundleData), perMeshBundleDataSize,
		m_perMeshBundleSharedData.bufferData, m_perMeshBundleSharedData.offset, tempBuffer
	);

	stagingBufferMan.AddBuffer(
		std::move(perMeshBufferData), perMeshDataSize,
		m_perMeshSharedData.bufferData, m_perMeshSharedData.offset, tempBuffer
	);

	_setMeshBundle(
		std::move(meshBundle),
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		perMeshletSharedBuffer, tempBuffer
	);
}
}
