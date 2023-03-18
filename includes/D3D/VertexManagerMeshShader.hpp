#ifndef VERTEX_MANAGER_MESH_SHADER_HPP_
#define VERTEX_MANAGER_MESH_SHADER_HPP_
#include <atomic>
#include <D3DDescriptorView.hpp>
#include <RootSignatureDynamic.hpp>
#include <UploadContainer.hpp>
#include <IModel.hpp>

class VertexManagerMeshShader {
public:
	VertexManagerMeshShader() noexcept;

	void AddGVerticesAndPrimIndices(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
		std::vector<std::uint32_t>&& gPrimIndices
	) noexcept;

	void BindVertexBuffers(
		ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex,
		const RSLayoutType& graphicsRSLayout
	) const noexcept;

	void ReserveBuffers(ID3D12Device* device) noexcept;
	void CreateBuffers(ID3D12Device* device);
	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

private:
	D3DUploadResourceDescriptorView m_vertexBuffer;
	D3DUploadResourceDescriptorView m_vertexIndicesBuffer;
	D3DUploadResourceDescriptorView m_primIndicesBuffer;
	std::vector<Vertex> m_gVertices;
	std::vector<std::uint32_t> m_gVerticesIndices;
	std::vector<std::uint32_t> m_gPrimIndices;
};
#endif
