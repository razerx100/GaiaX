#ifndef VERTEX_MANAGER_VERTEX_HPP_
#define VERTEX_MANAGER_VERTEX_HPP_
#include <VertexManager.hpp>
#include <BufferView.hpp>
#include <D3DResourceManager.hpp>
#include <UploadContainer.hpp>

class VertexManagerVertex final : public VertexManager {
public:
	VertexManagerVertex() noexcept;

	void AddGlobalVertices(
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	) noexcept override;

	void BindVertices(ID3D12GraphicsCommandList* graphicsCmdList) const noexcept override;

	void CreateBuffers(ID3D12Device* device) override;
	void ReserveBuffers(ID3D12Device* device) override;
	void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept override;
	void ReleaseUploadResources() noexcept override;
	void CopyData(std::atomic_size_t& workCount) const noexcept override;

private:
	void SetMemoryAddresses() noexcept;

private:
	BufferView<D3D12_VERTEX_BUFFER_VIEW> m_gVertexBufferView;
	BufferView<D3D12_INDEX_BUFFER_VIEW> m_gIndexBufferView;
	D3DUploadableResourceManager m_vertexBuffer;
	std::unique_ptr<UploadContainer> m_vertexUploadContainer;
};
#endif
