#ifndef VERTEX_MANAGER_HPP_
#define VERTEX_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <memory>
#include <cstdint>
#include <atomic>

class VertexManager {
public:
	virtual ~VertexManager() = default;

	virtual void AddGlobalVertices(
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	) noexcept = 0;

	virtual void BindVertices(ID3D12GraphicsCommandList* graphicsCmdList) const noexcept = 0;

	virtual void CreateBuffers(ID3D12Device* device) = 0;
	virtual void ReserveBuffers(ID3D12Device* device) = 0;
	virtual void RecordResourceUploads(ID3D12GraphicsCommandList* copyList) noexcept = 0;
	virtual void ReleaseUploadResources() noexcept = 0;
	virtual void CopyData(std::atomic_size_t& workCount) const noexcept = 0;
};
#endif
