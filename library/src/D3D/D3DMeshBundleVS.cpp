#include <D3DMeshBundleVS.hpp>
#include <VectorToSharedPtr.hpp>

namespace Gaia
{
D3DMeshBundleVS::D3DMeshBundleVS()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u }, m_indexBufferSharedData{ nullptr, 0u, 0u },
	m_perMeshSharedData{ nullptr, 0u, 0u }, m_perMeshBundleSharedData{ nullptr, 0u, 0u },
	m_bundleDetails {}
{}

void D3DMeshBundleVS::_setMeshBundle(
	MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	// Vertex Buffer
	{
		const std::vector<Vertex>& vertices = meshBundle.vertices;

		const auto vertexBufferSize = static_cast<UINT64>(sizeof(Vertex) * std::size(vertices));

		m_vertexBufferSharedData = vertexSharedBuffer.AllocateAndGetSharedData(
			vertexBufferSize, tempBuffer
		);

		std::shared_ptr<std::uint8_t[]> vertexBufferData
			= Callisto::CopyVectorToSharedPtr(vertices);

		stagingBufferMan.AddBuffer(
			std::move(vertexBufferData), vertexBufferSize,
			m_vertexBufferSharedData.bufferData, m_vertexBufferSharedData.offset,
			tempBuffer
		);
	}

	// Index Buffer
	{
		const std::vector<std::uint32_t>& indices = meshBundle.indices;

		const auto indexBufferSize = static_cast<UINT64>(
			sizeof(std::uint32_t) * std::size(indices)
		);

		m_indexBufferSharedData = indexSharedBuffer.AllocateAndGetSharedData(
			indexBufferSize, tempBuffer
		);

		std::shared_ptr<std::uint8_t[]> indexBufferData = Callisto::CopyVectorToSharedPtr(indices);

		stagingBufferMan.AddBuffer(
			std::move(indexBufferData), indexBufferSize,
			m_indexBufferSharedData.bufferData, m_indexBufferSharedData.offset,
			tempBuffer
		);
	}

	m_bundleDetails = std::move(meshBundle.bundleDetails.meshTemporaryDetailsVS);
}

void D3DMeshBundleVS::SetMeshBundle(
	MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	_setMeshBundle(
		std::move(meshBundle), stagingBufferMan, vertexSharedBuffer, indexSharedBuffer,
		tempBuffer
	);
}

void D3DMeshBundleVS::SetMeshBundle(
	MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
	SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	constexpr size_t perMeshDataStride = sizeof(AxisAlignedBoundingBox);

	const std::vector<MeshTemporaryDetailsVS>& meshDetailsVS
		= meshBundle.bundleDetails.meshTemporaryDetailsVS;

	const size_t meshCount     = std::size(meshDetailsVS);
	const auto perMeshDataSize = static_cast<UINT64>(perMeshDataStride * meshCount);

	m_perMeshSharedData = perMeshSharedBuffer.AllocateAndGetSharedData(
		perMeshDataSize, tempBuffer
	);

	auto perMeshBufferData = std::make_shared<std::uint8_t[]>(perMeshDataSize);

	{
		size_t perMeshOffset             = 0u;
		std::uint8_t* perMeshBufferStart = perMeshBufferData.get();

		for (const MeshTemporaryDetailsVS& meshDetail : meshDetailsVS)
		{
			memcpy(perMeshBufferStart + perMeshOffset, &meshDetail.aabb, perMeshDataStride);

			perMeshOffset += perMeshDataStride;
		}
	}

	// Mesh Bundle Data
	constexpr size_t perMeshBundleDataSize = sizeof(PerMeshBundleData);

	auto perBundleData = std::make_shared<std::uint8_t[]>(perMeshBundleDataSize);

	m_perMeshBundleSharedData = perMeshBundleSharedBuffer.AllocateAndGetSharedData(
		perMeshBundleDataSize, tempBuffer
	);

	{
		PerMeshBundleData bundleData
		{
			.meshOffset = static_cast<std::uint32_t>(
				m_perMeshSharedData.offset / perMeshDataStride
			)
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
		std::move(meshBundle), stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer
	);
}

void D3DMeshBundleVS::Bind(const D3DCommandList& graphicsCmdList) const noexcept
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
}
