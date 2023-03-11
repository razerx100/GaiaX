#ifndef VERTEX_MANAGER_MESH_SHADER_HPP_
#define VERTEX_MANAGER_MESH_SHADER_HPP_
#include <D3DDescriptorView.hpp>
#include <IModel.hpp>

class VertexManagerMeshShader {
public:
	VertexManagerMeshShader() noexcept;

	void AddGVerticesAndPrimIndices(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
		std::vector<std::uint32_t>&& gPrimIndices
	) noexcept;

	void BindVertexBuffers(ID3D12GraphicsCommandList* graphicsCmdList) const noexcept;
};
#endif
