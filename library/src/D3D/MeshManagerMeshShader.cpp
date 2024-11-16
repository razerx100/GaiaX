#include <MeshManagerMeshShader.hpp>
#include <VectorToSharedPtr.hpp>

MeshManagerMeshShader::MeshManagerMeshShader()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u },
	m_vertexIndicesBufferSharedData{ nullptr, 0u, 0u }, m_primIndicesBufferSharedData{ nullptr, 0u, 0u },
	m_meshletBufferSharedData{ nullptr, 0u, 0u }, m_meshletBoundsSharedData{ nullptr, 0u, 0u },
	m_meshDetails{ 0u, 0u, 0u }, m_bundleDetails{}
{}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& meshletSharedBuffer,
	TemporaryDataBufferGPU& tempBuffer
) {
	const std::vector<Vertex>& vertices  = meshBundle->GetVertices();

	auto ConfigureBuffer = []<typename T>
		(
			const std::vector<T>& elements, StagingBufferManager& stagingBufferMan,
			SharedBufferGPU& sharedBuffer, SharedBufferData& sharedData,
			std::uint32_t& detailOffset, TemporaryDataBufferGPU& tempBuffer
		)
	{
		constexpr auto stride = static_cast<UINT64>(sizeof(T));
		const auto bufferSize = static_cast<UINT64>(stride * std::size(elements));

		sharedData   = sharedBuffer.AllocateAndGetSharedData(bufferSize, tempBuffer);
		detailOffset = static_cast<std::uint32_t>(sharedData.offset / stride);

		std::shared_ptr<std::uint8_t[]> tempDataBuffer = CopyVectorToSharedPtr(elements);

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
		m_meshDetails.vertexOffset, tempBuffer
	);
	ConfigureBuffer(
		vertexIndices, stagingBufferMan, vertexIndicesSharedBuffer, m_vertexIndicesBufferSharedData,
		m_meshDetails.vertexIndicesOffset, tempBuffer
	);
	ConfigureBuffer(
		primIndices, stagingBufferMan, primIndicesSharedBuffer, m_primIndicesBufferSharedData,
		m_meshDetails.primIndicesOffset, tempBuffer
	);
	ConfigureBuffer(
		meshletDetails, stagingBufferMan, meshletSharedBuffer, m_meshletBufferSharedData,
		m_meshDetails.meshletOffset, tempBuffer
	);

	m_bundleDetails = std::move(meshBundle->GetBundleDetails());
}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& meshletSharedBuffer,
	SharedBufferGPU& meshletBoundsSharedBuffer, TemporaryDataBufferGPU& tempBuffer
) {
	constexpr auto perMeshDataStride = sizeof(AxisAlignedBoundingBox);

	// Need this or else the overload which returns the R value ref will be called.
	const MeshBundleMS& meshBundleR             = *meshBundle;
	const std::vector<MeshDetails>& meshDetails = meshBundleR.GetBundleDetails().meshDetails;

	const size_t meshCount     = std::size(meshDetails);
	const auto perMeshDataSize = static_cast<UINT64>(perMeshDataStride * meshCount);

	m_meshletBoundsSharedData  = meshletBoundsSharedBuffer.AllocateAndGetSharedData(
		perMeshDataSize, tempBuffer
	);

	auto perMeshBufferData     = std::make_shared<std::uint8_t[]>(perMeshDataSize);

	{
		size_t perMeshOffset             = 0u;
		std::uint8_t* perMeshBufferStart = perMeshBufferData.get();

		for (const MeshDetails& meshDetail : meshDetails)
		{
			memcpy(perMeshBufferStart + perMeshOffset, &meshDetail.aabb, perMeshDataStride);

			perMeshOffset += perMeshDataStride;
		}
	}

	stagingBufferMan.AddBuffer(
		std::move(perMeshBufferData), perMeshDataSize,
		m_meshletBoundsSharedData.bufferData, m_meshletBoundsSharedData.offset, tempBuffer
	);

	SetMeshBundle(
		std::move(meshBundle),
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		meshletSharedBuffer, tempBuffer
	);
}
