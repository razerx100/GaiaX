#include <VertexBuffer.hpp>
#include <cstring>

VertexBuffer::VertexBuffer(
	ID3D12Device* device,
	const std::vector<Ceres::Float32_3>& vertices,
	const Ceres::VectorF32& solidColor
) : m_vertexBufferView{} {

	std::uint32_t vertexSize = static_cast<std::uint32_t>(sizeof(Ceres::Float32_3));
	std::uint32_t colorSize = static_cast<std::uint32_t>(sizeof(Ceres::VectorF32));
	std::uint64_t strideSize = static_cast<std::uint64_t>(vertexSize) + colorSize;
	std::uint64_t bufferSize = vertices.size() * strideSize;

	m_buffer.CreateBuffer(device, bufferSize);

	for (std::uint32_t index = 0u, writeLocation = 0u; index < vertices.size(); ++index) {
		std::memcpy(m_buffer.GetCPUHandle() + writeLocation, &vertices[index], vertexSize);
		writeLocation += vertexSize;

		std::memcpy(m_buffer.GetCPUHandle() + writeLocation, &solidColor, colorSize);
		writeLocation += colorSize;
	}

	m_vertexBufferView.BufferLocation = m_buffer.GetGPUHandle();
	m_vertexBufferView.SizeInBytes = bufferSize;
	m_vertexBufferView.StrideInBytes = strideSize;
}

VertexBuffer::VertexBuffer(
	ID3D12Device* device,
	const std::vector<Ceres::Float32_3>& vertices
) : m_vertexBufferView{} {

	std::uint32_t vertexSize = static_cast<std::uint32_t>(sizeof(Ceres::Float32_3));
	std::uint64_t strideSize = static_cast<std::uint64_t>(vertexSize);
	std::uint64_t bufferSize = vertices.size() * strideSize;

	m_buffer.CreateBuffer(device, bufferSize);

	for (std::uint32_t index = 0u, writeLocation = 0u,
		vertexSize = static_cast<std::uint32_t>(sizeof(Ceres::Float32_3)),
		colorSize = static_cast<std::uint32_t>(sizeof(Ceres::VectorF32));
		index < vertices.size(); ++index) {

		std::memcpy(m_buffer.GetCPUHandle() + writeLocation, &vertices[index], vertexSize);
		writeLocation += vertexSize;
	}

	m_vertexBufferView.BufferLocation = m_buffer.GetGPUHandle();
	m_vertexBufferView.SizeInBytes = bufferSize;
	m_vertexBufferView.StrideInBytes = strideSize;
}

D3D12_VERTEX_BUFFER_VIEW* VertexBuffer::GetVertexBufferRef() noexcept {
	return &m_vertexBufferView;
}
