#include <PerFrameBuffers.hpp>
#include <Gaia.hpp>

#include <CameraManager.hpp>

PerFrameBuffers::PerFrameBuffers() {
	InitBuffers();
}

void PerFrameBuffers::InitBuffers() {
	m_cameraEntity.Init(sizeof(DirectX::XMMATRIX) * 2u);
}

void PerFrameBuffers::SetMemoryAddresses() noexcept {
	m_cameraEntity.SetMemoryAddresses();
}

void PerFrameBuffers::BindPerFrameBuffers(
	ID3D12GraphicsCommandList* graphicsCmdList
) const noexcept {
	std::uint8_t* cameraCpuHandle = m_cameraEntity.GetCpuHandle();

	Gaia::cameraManager->CopyData(cameraCpuHandle);

	graphicsCmdList->SetGraphicsRootConstantBufferView(
		4u, m_cameraEntity.GetGpuHandle()
	);
}

// Per Frame Entity
void PerFrameBuffers::PerFrameEntity::Init(size_t bufferSize) noexcept {
	m_sharedMemoryHandles = Gaia::constantBuffer->GetSharedAddresses(bufferSize);
}

void PerFrameBuffers::PerFrameEntity::SetMemoryAddresses() noexcept {
	auto& [cpuSharedHandle, gpuSharedHandle] = m_sharedMemoryHandles;

	m_pCpuHandle = reinterpret_cast<std::uint8_t*>(static_cast<std::uint64_t>(*cpuSharedHandle));
	m_gpuHandle = *gpuSharedHandle;
}

std::uint8_t* PerFrameBuffers::PerFrameEntity::GetCpuHandle() const noexcept {
	return m_pCpuHandle;
}

D3D12_GPU_VIRTUAL_ADDRESS PerFrameBuffers::PerFrameEntity::GetGpuHandle() const noexcept {
	return m_gpuHandle;
}
