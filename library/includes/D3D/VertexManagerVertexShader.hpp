#ifndef VERTEX_MANAGER_VERTEX_SHADER_HPP_
#define VERTEX_MANAGER_VERTEX_SHADER_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <cstdint>
#include <atomic>
#include <D3DResourceBuffer.hpp>
#include <IModel.hpp>

class VertexManagerVertexShader {
public:
	VertexManagerVertexShader() noexcept;

	void AddGVerticesAndIndices(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
	) noexcept;

	void BindVertexAndIndexBuffer(ID3D12GraphicsCommandList* graphicsCmdList) const noexcept;

	void CreateBuffers(ID3D12Device* device);
	void ReserveBuffers(ID3D12Device* device);
	void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResources() noexcept;

private:
	D3DUploadableResourceBuffer m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_gVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_gIndexBufferView;
	std::vector<Vertex> m_gVertices;
	std::vector<std::uint32_t> m_gIndices;
	size_t m_verticesOffset;
	size_t m_indicesOffset;
};
#endif
