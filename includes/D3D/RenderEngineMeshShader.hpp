#ifndef RENDER_ENGINE_MESH_SHADER_HPP_
#define RENDER_ENGINE_MESH_SHADER_HPP_
#include <memory>
#include <RenderEngineBase.hpp>
#include <VertexManagerMeshShader.hpp>
#include <GraphicsPipelineMeshShader.hpp>
#include <optional>

class RenderEngineMeshDraw : public RenderEngineBase
{
public:
	RenderEngineMeshDraw(ID3D12Device* device) noexcept;

	void AddMeshletModelSet(
		std::vector<MeshletModel>& meshletModels, const std::wstring& pixelShader
	) noexcept final;
	void AddGVerticesAndPrimIndices(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
		std::vector<std::uint32_t>&& gPrimIndices
	) noexcept final;
	void ExecuteRenderStage(size_t frameIndex) final;
	void UpdateModelBuffers(size_t frameIndex) const noexcept final;

	void CreateBuffers(ID3D12Device* device) final;
	void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept final;
	void ReleaseUploadResources() noexcept final;
	void ConstructPipelines() final;

private:
	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineMeshShader>;

	void ReserveBuffersDerived(ID3D12Device* device) final;

	void RecordDrawCommands(
		ID3D12GraphicsCommandList6* graphicsCommandList, size_t frameIndex
	);
	void BindGraphicsBuffers(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	);

	[[nodiscard]]
	std::unique_ptr<RootSignatureDynamic> CreateGraphicsRootSignature(
		ID3D12Device* device
	) const noexcept final;

private:
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;

	VertexManagerMeshShader m_vertexManager;
	D3DUploadResourceDescriptorView m_meshletBuffer;
	std::vector<Meshlet> m_meshlets;
};
#endif
