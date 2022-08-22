#ifndef PER_FRAME_BUFFERS_HPP_
#define PER_FRAME_BUFFERS_HPP_
#include <D3DHeaders.hpp>
#include <BufferView.hpp>
#include <D3DDescriptorView.hpp>
#include <MemoryContainer.hpp>

class PerFrameBuffers {
public:
	PerFrameBuffers(std::uint32_t frameCount);

	void BindPerFrameBuffers(
		ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex
	) const noexcept;
	void SetMemoryAddresses() noexcept;

	void AddModelInputs(
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	);

	void CopyData() noexcept;

private:
	void InitBuffers(std::uint32_t frameCount);

private:
	D3DRootDescriptorView m_cameraBuffer;
	BufferView<D3D12_VERTEX_BUFFER_VIEW> m_gVertexBufferView;
	BufferView<D3D12_INDEX_BUFFER_VIEW> m_gIndexBufferView;
	std::unique_ptr<MemoryContainer> m_gVertexData;
	std::unique_ptr<MemoryContainer> m_gIndexData;
	size_t m_vertexOffset;
	size_t m_indexOffset;
};
#endif