#include <VertexManagerVertexShader.hpp>
#include <Gaia.hpp>

VertexManagerVertexShader::VertexManagerVertexShader() noexcept
	: m_gVertexBufferView{}, m_gIndexBufferView{},
	m_vertexUploadContainer{ std::make_unique<UploadContainer>() } {}

void VertexManagerVertexShader::AddGVerticesAndIndices(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
) noexcept {
	const size_t vertexBufferSize = sizeof(Vertex) * std::size(gVertices);
	const size_t indexBufferSize = sizeof(std::uint32_t) * std::size(gIndices);

	const size_t vertexOffset = m_vertexBuffer.ReserveSpaceAndGetOffset(
		vertexBufferSize
	);
	const size_t indexOffset = m_vertexBuffer.ReserveSpaceAndGetOffset(
		indexBufferSize
	);

	m_vertexUploadContainer->AddMemory(std::data(gVertices), vertexBufferSize, vertexOffset);
	m_vertexUploadContainer->AddMemory(std::data(gIndices), indexBufferSize, indexOffset);

	m_gIndices = gIndices;
	m_gVertices = gVertices;

	m_gVertexBufferView.AddBufferView(
		D3D12_VERTEX_BUFFER_VIEW{
			static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(vertexOffset),
			static_cast<UINT>(vertexBufferSize), static_cast<UINT>(sizeof(Vertex))
		}
	);

	m_gIndexBufferView.AddBufferView(
		D3D12_INDEX_BUFFER_VIEW{
			static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(indexOffset),
			static_cast<UINT>(indexBufferSize), DXGI_FORMAT_R32_UINT
		}
	);
}

void VertexManagerVertexShader::BindVertexAndIndexBuffer(
	ID3D12GraphicsCommandList* graphicsCmdList
) const noexcept {
	graphicsCmdList->IASetVertexBuffers(0u, 1u, m_gVertexBufferView.GetAddress());
	graphicsCmdList->IASetIndexBuffer(m_gIndexBufferView.GetAddress());
}

void VertexManagerVertexShader::SetMemoryAddresses() noexcept {
	std::uint8_t* vertexBufferUploadStartAddress = m_vertexBuffer.GetCPUStartAddress();
	m_vertexUploadContainer->SetStartingAddress(vertexBufferUploadStartAddress);

	const D3D12_GPU_VIRTUAL_ADDRESS vertexGpuStart = m_vertexBuffer.GetGPUStartAddress();

	m_gVertexBufferView.OffsetGPUAddress(vertexGpuStart);
	m_gIndexBufferView.OffsetGPUAddress(vertexGpuStart);
}

void VertexManagerVertexShader::CreateBuffers(ID3D12Device* device) {
	m_vertexBuffer.CreateResource(device);

	SetMemoryAddresses();
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
	m_vertexUploadContainer.reset();

	m_gIndices = std::vector<std::uint32_t>{};
	m_gVertices = std::vector<Vertex>{};
}

void VertexManagerVertexShader::CopyData(std::atomic_size_t& workCount) const noexcept {
	m_vertexUploadContainer->CopyData(workCount);
}
