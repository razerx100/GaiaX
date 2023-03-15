#include <ranges>
#include <algorithm>
#include <RenderEngineMeshShader.hpp>
#include <Gaia.hpp>

void RenderEngineMeshShader::ExecuteRenderStage(size_t frameIndex) {
	ID3D12GraphicsCommandList6* graphicsCommandList = Gaia::graphicsCmdList->GetCommandList6();

	ExecutePreGraphicsStage(graphicsCommandList, frameIndex);
	RecordDrawCommands(graphicsCommandList, frameIndex);
}

void RenderEngineMeshShader::RecordDrawCommands(
	ID3D12GraphicsCommandList6* graphicsCommandList, size_t frameIndex
) {

}

void RenderEngineMeshShader::AddMeshletModelSet(
	std::vector<MeshletModel>&& meshletModels, const std::wstring& pixelShader
) noexcept {
	for (size_t index = 0u; index < std::size(meshletModels); ++index) {
		std::vector<Meshlet>&& meshlets = std::move(meshletModels[index].meshlets);

		std::ranges::move(meshlets, std::back_inserter(m_meshlets));
	}
}

void RenderEngineMeshShader::AddGVerticesAndPrimIndices(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
	std::vector<std::uint32_t>&& gPrimIndices
) noexcept {
	m_vertexManager.AddGVerticesAndPrimIndices(
		std::move(gVertices), std::move(gVerticesIndices), std::move(gPrimIndices)
	);
}

void RenderEngineMeshShader::CreateBuffers(ID3D12Device* device) {
	m_vertexManager.CreateBuffers(device);

	CreateUploadDescView(device, m_meshletBuffer, m_meshlets);
}

void RenderEngineMeshShader::RecordResourceUploads(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_vertexManager.RecordResourceUpload(copyList);
	m_meshletBuffer.RecordResourceUpload(copyList);
}

void RenderEngineMeshShader::ReleaseUploadResources() noexcept {
	m_vertexManager.ReleaseUploadResource();
	m_meshletBuffer.ReleaseUploadResource();

	m_meshlets = std::vector<Meshlet>{};
}

void RenderEngineMeshShader::ReserveBuffersDerived(ID3D12Device* device) {
	m_vertexManager.ReserveBuffers(device);

	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	const size_t meshletDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();

	SetDescBufferInfo(device, meshletDescriptorOffset, m_meshlets, m_meshletBuffer);
}
