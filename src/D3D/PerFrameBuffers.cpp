#include <PerFrameBuffers.hpp>
#include <Gaia.hpp>
#include <RootSignatureDynamic.hpp>

#include <CameraManager.hpp>

PerFrameBuffers::PerFrameBuffers(std::uint32_t frameCount) {
	InitBuffers(frameCount);
}

void PerFrameBuffers::InitBuffers(std::uint32_t frameCount) {
	size_t cameraBufferSize = sizeof(DirectX::XMMATRIX) * 2u;
	constexpr size_t cameraBufferAlignment = 256u;
	size_t cameraOffset = Gaia::Resources::cpuWriteBuffer->ReserveSpaceAndGetOffset(
		cameraBufferSize, frameCount, cameraBufferAlignment
	);

	m_cameraBuffer.SetAddressesStart(cameraOffset, cameraBufferSize, cameraBufferAlignment);
}

void PerFrameBuffers::SetMemoryAddresses() noexcept {
	std::uint8_t* cpuOffset = Gaia::Resources::cpuWriteBuffer->GetCPUStartAddress();
	D3D12_GPU_VIRTUAL_ADDRESS gpuOffset = Gaia::Resources::cpuWriteBuffer->GetGPUStartAddress();

	m_cameraBuffer.UpdateCPUAddressStart(cpuOffset);
	m_cameraBuffer.UpdateGPUAddressStart(gpuOffset);

	const D3D12_GPU_VIRTUAL_ADDRESS vertexGpuStart =
		Gaia::Resources::vertexBuffer->GetGPUStartAddress();

	m_gVertexBufferView.OffsetGPUAddress(vertexGpuStart);
	m_gIndexBufferView.OffsetGPUAddress(vertexGpuStart);
}

void PerFrameBuffers::UpdateData(size_t frameIndex) const noexcept {
	std::uint8_t* cameraCpuHandle = m_cameraBuffer.GetCPUAddressStart(frameIndex);

	Gaia::cameraManager->CopyData(cameraCpuHandle);
}

void PerFrameBuffers::BindPerFrameBuffers(
	ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex,
	const std::vector<UINT>& rsLayout
) const noexcept {
	static constexpr size_t cameraTypeIndex = static_cast<size_t>(RootSigElement::Camera);

	graphicsCmdList->SetGraphicsRootConstantBufferView(
		rsLayout[cameraTypeIndex], m_cameraBuffer.GetGPUAddressStart(frameIndex)
	);
}

void PerFrameBuffers::BindVertexBuffer(
	ID3D12GraphicsCommandList* graphicsCmdList
) const noexcept {
	graphicsCmdList->IASetVertexBuffers(0u, 1u, m_gVertexBufferView.GetAddress());
	graphicsCmdList->IASetIndexBuffer(m_gIndexBufferView.GetAddress());
}

void PerFrameBuffers::AddModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	const size_t vertexOffset = Gaia::Resources::vertexBuffer->ReserveSpaceAndGetOffset(
		vertexBufferSize
	);
	const size_t indexOffset = Gaia::Resources::vertexBuffer->ReserveSpaceAndGetOffset(
		indexBufferSize
	);

	Gaia::Resources::vertexUploadContainer->AddMemory(
		std::move(vertices), vertexBufferSize, vertexOffset
	);
	Gaia::Resources::vertexUploadContainer->AddMemory(
		std::move(indices), indexBufferSize, indexOffset
	);

	m_gVertexBufferView.AddBufferView(
		D3D12_VERTEX_BUFFER_VIEW{
			static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(vertexOffset),
			static_cast<UINT>(vertexBufferSize), static_cast<UINT>(strideSize)
		}
	);

	m_gIndexBufferView.AddBufferView(
		D3D12_INDEX_BUFFER_VIEW{
			static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(indexOffset),
			static_cast<UINT>(indexBufferSize), DXGI_FORMAT_R32_UINT
		}
	);
}
