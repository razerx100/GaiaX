#ifndef RENDER_ENGINE_MESH_SHADER_HPP_
#define RENDER_ENGINE_MESH_SHADER_HPP_
#include <RenderEngineBase.hpp>

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

private:
	void RecordDrawCommands(
		ID3D12GraphicsCommandList6* graphicsCommandList, size_t frameIndex
	);
};
#endif
