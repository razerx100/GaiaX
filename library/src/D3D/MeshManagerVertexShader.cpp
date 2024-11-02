#include <MeshManagerVertexShader.hpp>
#include <VectorToSharedPtr.hpp>

MeshManagerVertexShader::MeshManagerVertexShader()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u }, m_indexBufferSharedData{ nullptr, 0u, 0u },
	m_meshBoundsSharedData{ nullptr, 0u, 0u }, m_bundleDetails{}
{}

void MeshManagerVertexShader::SetMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
	TemporaryDataBufferGPU& tempBuffer
) {
	// Vertex Buffer
	{
		const std::vector<Vertex>& vertices = meshBundle->GetVertices();
		const auto vertexBufferSize         = static_cast<UINT64>(sizeof(Vertex) * std::size(vertices));

		m_vertexBufferSharedData = vertexSharedBuffer.AllocateAndGetSharedData(
			vertexBufferSize, tempBuffer
		);

		std::shared_ptr<std::uint8_t[]> vertexBufferData = CopyVectorToSharedPtr(vertices);

		stagingBufferMan.AddBuffer(
			std::move(vertexBufferData), vertexBufferSize,
			m_vertexBufferSharedData.bufferData, m_vertexBufferSharedData.offset,
			tempBuffer
		);
	}

	// Index Buffer
	{
		const std::vector<std::uint32_t>& indices = meshBundle->GetIndices();
		const auto indexBufferSize                = sizeof(std::uint32_t) * std::size(indices);

		m_indexBufferSharedData = indexSharedBuffer.AllocateAndGetSharedData(indexBufferSize, tempBuffer);

		std::shared_ptr<std::uint8_t[]> indexBufferData = CopyVectorToSharedPtr(indices);

		stagingBufferMan.AddBuffer(
			std::move(indexBufferData), indexBufferSize,
			m_indexBufferSharedData.bufferData, m_indexBufferSharedData.offset,
			tempBuffer
		);
	}

	m_bundleDetails = std::move(meshBundle->GetBundleDetails());
}

void MeshManagerVertexShader::SetMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
	SharedBufferGPU& boundsSharedBuffer, TemporaryDataBufferGPU& tempBuffer
) {
	constexpr auto boundStride = sizeof(AxisAlignedBoundingBox);

	// Need this or else the overload which returns the R value ref will be called.
	const MeshBundleVS& meshBundleR             = *meshBundle;
	const std::vector<MeshDetails>& meshDetails = meshBundleR.GetBundleDetails().meshDetails;

	const size_t meshCount = std::size(meshDetails);
	const auto boundSize   = static_cast<UINT64>(boundStride * meshCount);

	m_meshBoundsSharedData = boundsSharedBuffer.AllocateAndGetSharedData(boundSize, tempBuffer);

	auto boundBufferData   = std::make_shared<std::uint8_t[]>(boundSize);

	{
		size_t boundOffset             = 0u;
		std::uint8_t* boundBufferStart = boundBufferData.get();

		for (const MeshDetails& meshDetail : meshDetails)
		{
			memcpy(boundBufferStart + boundOffset, &meshDetail.aabb, boundStride);

			boundOffset += boundStride;
		}
	}

	stagingBufferMan.AddBuffer(
		std::move(boundBufferData), boundSize,
		m_meshBoundsSharedData.bufferData, m_meshBoundsSharedData.offset, tempBuffer
	);

	SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer
	);
}

void MeshManagerVertexShader::Bind(const D3DCommandList& graphicsCmdList) const noexcept
{
	ID3D12GraphicsCommandList* cmdList = graphicsCmdList.Get();

	{
		Buffer const* vertexBuffer = m_vertexBufferSharedData.bufferData;

		D3D12_VERTEX_BUFFER_VIEW vbv
		{
			.BufferLocation = vertexBuffer->GetGPUAddress() + m_vertexBufferSharedData.offset,
			.SizeInBytes    = static_cast<UINT>(m_vertexBufferSharedData.size),
			.StrideInBytes  = static_cast<UINT>(sizeof(Vertex))
		};

		cmdList->IASetVertexBuffers(0u, 1u, &vbv);
	}

	{
		Buffer const* indexBuffer = m_indexBufferSharedData.bufferData;

		D3D12_INDEX_BUFFER_VIEW ibv
		{
			.BufferLocation = indexBuffer->GetGPUAddress() + m_indexBufferSharedData.offset,
			.SizeInBytes    = static_cast<UINT>(m_indexBufferSharedData.size),
			.Format         = DXGI_FORMAT_R32_UINT
		};

		cmdList->IASetIndexBuffer(&ibv);
	}
}
