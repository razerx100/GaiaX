#include <PerFrameBuffers.hpp>
#include <Gaia.hpp>

#include <CameraManager.hpp>

PerFrameBuffers::PerFrameBuffers(std::uint32_t frameCount) {
	InitBuffers(frameCount);
}

void PerFrameBuffers::InitBuffers(std::uint32_t frameCount) {
	m_cameraBuffer.Init(sizeof(DirectX::XMMATRIX) * 2u, frameCount);
}

void PerFrameBuffers::SetMemoryAddresses() noexcept {
	m_cameraBuffer.SetMemoryAddresses();
	m_gVertexBufferView.SetGPUAddress();
	m_gIndexBufferView.SetGPUAddress();
}

void PerFrameBuffers::BindPerFrameBuffers(
	ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex
) const noexcept {
	std::uint8_t* cameraCpuHandle = m_cameraBuffer.GetCpuHandle(frameIndex);

	Gaia::cameraManager->CopyData(cameraCpuHandle);

	graphicsCmdList->SetGraphicsRootConstantBufferView(
		2u, m_cameraBuffer.GetGpuHandle(frameIndex)
	);

	graphicsCmdList->IASetVertexBuffers(0u, 1u, m_gVertexBufferView.GetAddress());
	graphicsCmdList->IASetIndexBuffer(m_gIndexBufferView.GetAddress());
}

void PerFrameBuffers::AddModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	auto vertexBufferAddress = Gaia::vertexBuffer->AddDataAndGetSharedAddress(
		std::move(vertices), vertexBufferSize
	);
	auto indexBufferAddress = Gaia::indexBuffer->AddDataAndGetSharedAddress(
		std::move(indices), indexBufferSize
	);

	m_gVertexBufferView.AddBufferView(
		D3D12_VERTEX_BUFFER_VIEW{
			0u, static_cast<UINT>(vertexBufferSize), static_cast<UINT>(strideSize)
		}
	);
	m_gVertexBufferView.AddSharedAddress(std::move(vertexBufferAddress));

	m_gIndexBufferView.AddBufferView(
		D3D12_INDEX_BUFFER_VIEW{
			0u, static_cast<UINT>(indexBufferSize), DXGI_FORMAT_R32_UINT
		}
	);
	m_gIndexBufferView.AddSharedAddress(std::move(indexBufferAddress));
}
