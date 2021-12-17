#ifndef __VERTEX_SHADER_HPP__
#define __VERTEX_SHADER_HPP__
#include <IVertexBuffer.hpp>
#include <IModel.hpp>
#include <vector>
#include <UploadBuffer.hpp>

class VertexBuffer : public IVertexBuffer {
public:
	VertexBuffer(
		ID3D12Device* device,
		const std::vector<Ceres::Float32_3>& vertices,
		const Ceres::VectorF32& solidColor
	);
	VertexBuffer(
		ID3D12Device* device,
		const std::vector<Ceres::Float32_3>& vertices
	);

	D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferRef() noexcept override;

private:
	UploadBuffer m_buffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
};
#endif
