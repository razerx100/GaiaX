#include <PerFrameBuffers.hpp>
#include <Gaia.hpp>

#include <CameraManager.hpp>

PerFrameBuffers::PerFrameBuffers() {
	InitBuffers();
}

void PerFrameBuffers::InitBuffers() {
	m_sharedMemoryHandles = Gaia::constantBuffer->GetSharedAddresses(
		sizeof(DirectX::XMMATRIX) * 2u
	);
}

void PerFrameBuffers::SetMemoryAddresses() noexcept {
	auto& [cpuSharedHandle, gpuSharedHandle] = m_sharedMemoryHandles;

	m_pCpuHandle = reinterpret_cast<std::uint8_t*>(static_cast<std::uint64_t>(*cpuSharedHandle));
	m_gpuHandle = *gpuSharedHandle;
}

void PerFrameBuffers::BindPerFrameBuffers(
	ID3D12GraphicsCommandList* graphicsCmdList
) const noexcept {
	Gaia::cameraManager->CopyData(m_pCpuHandle);

	graphicsCmdList->SetGraphicsRootConstantBufferView(4u, m_gpuHandle);
}
