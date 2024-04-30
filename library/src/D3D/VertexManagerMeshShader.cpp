#include <VertexManagerMeshShader.hpp>
#include <Gaia.hpp>

VertexManagerMeshShader::VertexManagerMeshShader() noexcept
	: m_vertexBuffer{ DescriptorType::SRV }, m_vertexIndicesBuffer{ DescriptorType::SRV },
	m_primIndicesBuffer{ DescriptorType::SRV } {}

void VertexManagerMeshShader::AddGVerticesAndPrimIndices(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
	std::vector<std::uint32_t>&& gPrimIndices
) noexcept {
	m_gVertices = std::move(gVertices);
	m_gVerticesIndices = std::move(gVerticesIndices);
	m_gPrimIndices = std::move(gPrimIndices);
}

void VertexManagerMeshShader::BindVertexBuffers(
	ID3D12GraphicsCommandList* graphicsCmdList, const RSLayoutType& graphicsRSLayout
) const noexcept {
	static constexpr auto vertexRIndex = static_cast<size_t>(RootSigElement::VertexData);
	graphicsCmdList->SetGraphicsRootDescriptorTable(
		graphicsRSLayout[vertexRIndex], m_vertexBuffer.GetFirstGPUDescriptorHandle()
	);

	static constexpr auto vertexIndicesRIndex =
		static_cast<size_t>(RootSigElement::VertexIndices);
	graphicsCmdList->SetGraphicsRootDescriptorTable(
		graphicsRSLayout[vertexIndicesRIndex],
		m_vertexIndicesBuffer.GetFirstGPUDescriptorHandle()
	);

	static constexpr auto primIndicesRIndex = static_cast<size_t>(RootSigElement::PrimIndices);
	graphicsCmdList->SetGraphicsRootDescriptorTable(
		graphicsRSLayout[primIndicesRIndex], m_primIndicesBuffer.GetFirstGPUDescriptorHandle()
	);
}

void VertexManagerMeshShader::ReserveBuffers(ID3D12Device* device) noexcept {
	const size_t vertexDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();
	SetDescBufferInfo(device, vertexDescriptorOffset, m_gVertices, m_vertexBuffer);

	const size_t vertexIndicesDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();
	SetDescBufferInfo(
		device, vertexIndicesDescriptorOffset, m_gVerticesIndices, m_vertexIndicesBuffer
	);

	const size_t primIndicesDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();
	SetDescBufferInfo(
		device, primIndicesDescriptorOffset, m_gPrimIndices, m_primIndicesBuffer
	);
}

void VertexManagerMeshShader::CreateBuffers(ID3D12Device* device) {
	CreateUploadDescView(device, m_vertexBuffer, m_gVertices);

	CreateUploadDescView(device, m_vertexIndicesBuffer,m_gVerticesIndices);

	CreateUploadDescView(device, m_primIndicesBuffer, m_gPrimIndices);
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
