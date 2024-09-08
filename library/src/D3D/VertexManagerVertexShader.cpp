#include <VertexManagerVertexShader.hpp>

/*
VertexManagerVertexShader::VertexManagerVertexShader() noexcept
	: m_gVertexBufferView{}, m_gIndexBufferView{}, m_verticesOffset{ 0u },
	m_indicesOffset{ 0u } {}

void VertexManagerVertexShader::AddGVerticesAndIndices(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
) noexcept {
	const auto vertexStrideSize = static_cast<UINT>(sizeof(Vertex));
	const size_t vertexBufferSize = vertexStrideSize * std::size(gVertices);
	const size_t indexBufferSize = sizeof(std::uint32_t) * std::size(gIndices);

	const size_t vertexOffset = m_vertexBuffer.ReserveSpaceAndGetOffset(
		vertexBufferSize
	);
	const size_t indexOffset = m_vertexBuffer.ReserveSpaceAndGetOffset(
		indexBufferSize
	);

	m_gVertexBufferView = D3D12_VERTEX_BUFFER_VIEW{
		.BufferLocation = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(vertexOffset),
		.SizeInBytes = static_cast<UINT>(vertexBufferSize),
		.StrideInBytes = vertexStrideSize
	};

	m_gVertices = std::move(gVertices);

	m_gIndexBufferView = D3D12_INDEX_BUFFER_VIEW{
		.BufferLocation = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(indexOffset),
		.SizeInBytes = static_cast<UINT>(indexBufferSize),
		.Format = DXGI_FORMAT_R32_UINT
	};

	m_gIndices = std::move(gIndices);
}

void VertexManagerVertexShader::BindVertexAndIndexBuffer(
	ID3D12GraphicsCommandList* graphicsCmdList
) const noexcept {
	graphicsCmdList->IASetVertexBuffers(0u, 1u, &m_gVertexBufferView);
	graphicsCmdList->IASetIndexBuffer(&m_gIndexBufferView);
}

void VertexManagerVertexShader::CreateBuffers(ID3D12Device* device) {
	m_vertexBuffer.CreateResource(device);

	std::uint8_t* vertexCpuStart = m_vertexBuffer.GetCPUStartAddress();

	Gaia::Resources::uploadContainer->AddMemory(
		std::data(m_gVertices), vertexCpuStart + m_gVertexBufferView.BufferLocation,
		m_gVertexBufferView.SizeInBytes
	);

	Gaia::Resources::uploadContainer->AddMemory(
		std::data(m_gIndices), vertexCpuStart + m_gIndexBufferView.BufferLocation,
		m_gIndexBufferView.SizeInBytes
	);

	const D3D12_GPU_VIRTUAL_ADDRESS vertexGpuStart = m_vertexBuffer.GetGPUStartAddress();

	m_gVertexBufferView.BufferLocation += vertexGpuStart;
	m_gIndexBufferView.BufferLocation += vertexGpuStart;
}

void VertexManagerVertexShader::ReserveBuffers(ID3D12Device* device) {
	m_vertexBuffer.ReserveHeapSpace(device);
}

void VertexManagerVertexShader::RecordResourceUploads(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_vertexBuffer.RecordResourceUpload(copyList);
}

void VertexManagerVertexShader::ReleaseUploadResources() noexcept {
	m_vertexBuffer.ReleaseUploadResource();

	m_gIndices = std::vector<std::uint32_t>{};
	m_gVertices = std::vector<Vertex>{};
}
*/
