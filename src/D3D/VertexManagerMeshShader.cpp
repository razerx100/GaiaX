#include <VertexManagerMeshShader.hpp>
#include <Gaia.hpp>

VertexManagerMeshShader::VertexManagerMeshShader() noexcept
	: m_vertexBuffer{ DescriptorType::SRV },
	m_vertexIndicesBuffer{ DescriptorType::SRV }, m_primIndicesBuffer{ DescriptorType::SRV } {}

void VertexManagerMeshShader::AddGVerticesAndPrimIndices(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
	std::vector<std::uint32_t>&& gPrimIndices
) noexcept {
	m_gVertices = std::move(gVertices);
	m_gVerticesIndices = std::move(gVerticesIndices);
	m_gPrimIndices = std::move(gPrimIndices);
}

void VertexManagerMeshShader::BindVertexBuffers(
	ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex
) const noexcept {
	static constexpr auto vertexRIndex = static_cast<size_t>(RootSigElement::VertexData);
	graphicsCmdList->SetGraphicsRootDescriptorTable(
		m_graphicsRSLayout[vertexRIndex],
		m_vertexBuffer.GetGPUDescriptorHandle(frameIndex)
	);

	static constexpr auto vertexIndicesRIndex =
		static_cast<size_t>(RootSigElement::VertexIndices);
	graphicsCmdList->SetGraphicsRootDescriptorTable(
		m_graphicsRSLayout[vertexIndicesRIndex],
		m_vertexIndicesBuffer.GetGPUDescriptorHandle(frameIndex)
	);

	static constexpr auto primIndicesRIndex = static_cast<size_t>(RootSigElement::PrimIndices);
	graphicsCmdList->SetGraphicsRootDescriptorTable(
		m_graphicsRSLayout[primIndicesRIndex],
		m_primIndicesBuffer.GetGPUDescriptorHandle(frameIndex)
	);
}

void VertexManagerMeshShader::SetGraphicsRootSignatureLayout(RSLayoutType rsLayout) noexcept {
	m_graphicsRSLayout = std::move(rsLayout);
}

void VertexManagerMeshShader::ReserveBuffers(ID3D12Device* device) noexcept {
	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	const size_t vertexDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();
	m_vertexBuffer.SetDescriptorOffset(vertexDescriptorOffset, descriptorSize);
	m_vertexBuffer.SetBufferInfo(
		device, static_cast<UINT64>(sizeof(Vertex)), static_cast<UINT>(std::size(m_gVertices))
	);

	const size_t vertexIndicesDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();
	m_vertexIndicesBuffer.SetDescriptorOffset(vertexIndicesDescriptorOffset, descriptorSize);
	m_vertexIndicesBuffer.SetBufferInfo(
		device, static_cast<UINT64>(sizeof(std::uint32_t)),
		static_cast<UINT>(std::size(m_gVerticesIndices))
	);

	const size_t primIndicesDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();
	m_primIndicesBuffer.SetDescriptorOffset(primIndicesDescriptorOffset, descriptorSize);
	m_primIndicesBuffer.SetBufferInfo(
		device, static_cast<UINT64>(sizeof(std::uint32_t)),
		static_cast<UINT>(std::size(m_gPrimIndices))
	);
}

void VertexManagerMeshShader::CreateBuffers(ID3D12Device* device) {
	const D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart =
		Gaia::descriptorTable->GetUploadDescriptorStart();

	const D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart =
		Gaia::descriptorTable->GetGPUDescriptorStart();

	CreateBuffer(
		device, uploadDescriptorStart, gpuDescriptorStart, m_vertexBuffer, m_gVertices
	);

	CreateBuffer(
		device, uploadDescriptorStart, gpuDescriptorStart, m_vertexIndicesBuffer,
		m_gVerticesIndices
	);

	CreateBuffer(
		device, uploadDescriptorStart, gpuDescriptorStart, m_primIndicesBuffer,
		m_gPrimIndices
	);
}

UploadContainer* VertexManagerMeshShader::GetGUploadContainer() const noexcept {
	return Gaia::Resources::uploadContainer.get();
}

void VertexManagerMeshShader::RecordResourceUpload(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_vertexBuffer.RecordResourceUpload(copyList);
	m_vertexIndicesBuffer.RecordResourceUpload(copyList);
	m_primIndicesBuffer.RecordResourceUpload(copyList);
}

void VertexManagerMeshShader::ReleaseUploadResource() noexcept {
	m_vertexBuffer.ReleaseUploadResource();
	m_vertexIndicesBuffer.ReleaseUploadResource();
	m_primIndicesBuffer.ReleaseUploadResource();

	m_gVertices = std::vector<Vertex>();
	m_gVerticesIndices = std::vector<std::uint32_t>();
	m_gPrimIndices = std::vector<std::uint32_t>();
}
