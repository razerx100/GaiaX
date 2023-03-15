#ifndef RENDER_ENGINE_MESH_SHADER_HPP_
#define RENDER_ENGINE_MESH_SHADER_HPP_
#include <RenderEngineBase.hpp>
#include <VertexManagerMeshShader.hpp>

class RenderEngineMeshShader : public RenderEngineBase {
public:
	void AddMeshletModelSet(
		std::vector<MeshletModel>&& meshletModels, const std::wstring& pixelShader
	) noexcept final;
	void AddGVerticesAndPrimIndices(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
		std::vector<std::uint32_t>&& gPrimIndices
	) noexcept final;
	void ExecuteRenderStage(size_t frameIndex) final;

	void CreateBuffers(ID3D12Device* device) final;
	void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept final;
	void ReleaseUploadResources() noexcept final;

private:
	void ReserveBuffersDerived(ID3D12Device* device) final;

	void RecordDrawCommands(
		ID3D12GraphicsCommandList6* graphicsCommandList, size_t frameIndex
	);

private:
	VertexManagerMeshShader m_vertexManager;
	D3DUploadResourceDescriptorView m_meshletBuffer;
	std::vector<Meshlet> m_meshlets;
};
#endif
