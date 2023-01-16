#include <VertexManagerVertex.hpp>
#include <Gaia.hpp>
#include <IModel.hpp>

VertexManagerVertex::VertexManagerVertex() noexcept
	: m_gVertexBufferView{}, m_gIndexBufferView{},
	m_vertexUploadContainer{ std::make_unique<UploadContainer>() } {}

void VertexManagerVertex::AddGlobalVertices(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) noexcept {
	const size_t vertexOffset = m_vertexBuffer.ReserveSpaceAndGetOffset(
		vertexBufferSize
	);
	const size_t indexOffset = m_vertexBuffer.ReserveSpaceAndGetOffset(
		indexBufferSize
	);

	m_vertexUploadContainer->AddMemory(std::move(vertices), vertexBufferSize, vertexOffset);
	m_vertexUploadContainer->AddMemory(std::move(indices), indexBufferSize, indexOffset);

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

void VertexManagerVertex::BindVertexAndIndexBuffer(
	ID3D12GraphicsCommandList* graphicsCmdList
) const noexcept {
	graphicsCmdList->IASetVertexBuffers(0u, 1u, m_gVertexBufferView.GetAddress());
	graphicsCmdList->IASetIndexBuffer(m_gIndexBufferView.GetAddress());
}

void VertexManagerVertex::SetMemoryAddresses() noexcept {
	std::uint8_t* vertexBufferUploadStartAddress = m_vertexBuffer.GetCPUStartAddress();
	m_vertexUploadContainer->SetStartingAddress(vertexBufferUploadStartAddress);

	const D3D12_GPU_VIRTUAL_ADDRESS vertexGpuStart = m_vertexBuffer.GetGPUStartAddress();

	m_gVertexBufferView.OffsetGPUAddress(vertexGpuStart);
	m_gIndexBufferView.OffsetGPUAddress(vertexGpuStart);
}

void VertexManagerVertex::CreateBuffers(ID3D12Device* device) {
	m_vertexBuffer.CreateResource(device);

	SetMemoryAddresses();
}

void VertexManagerVertex::ReserveBuffers(ID3D12Device* device) {
	m_vertexBuffer.ReserveHeapSpace(device);
}

void VertexManagerVertex::RecordResourceUploads(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_vertexBuffer.RecordResourceUpload(copyList);
}

void VertexManagerVertex::ReleaseUploadResources() noexcept {
	m_vertexBuffer.ReleaseUploadResource();
	m_vertexUploadContainer.reset();
}

void VertexManagerVertex::CopyData(std::atomic_size_t& workCount) const noexcept {
	m_vertexUploadContainer->CopyData(workCount);
}
