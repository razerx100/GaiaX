#include <CPUWriteAddressManager.hpp>
#include <D3DHelperFunctions.hpp>
#include <Gaia.hpp>

CPUWriteAddressManager::CPUWriteAddressManager() noexcept
	: m_pCpuHandle{ nullptr }, m_gpuHandle{}, m_perFrameBufferSize{ 0 } {}

void CPUWriteAddressManager::Init(size_t bufferSize, std::uint32_t frameCount) noexcept {
	m_perFrameBufferSize = Align(bufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	m_sharedBufferAddresses = Gaia::cpuWriteBuffer->GetSharedAddresses(
		m_perFrameBufferSize * frameCount
	);
}

void CPUWriteAddressManager::SetMemoryAddresses() noexcept {
	auto& [cpuSharedAddress, gpuSharedAddress] = m_sharedBufferAddresses;

	m_pCpuHandle =
		reinterpret_cast<std::uint8_t*>(static_cast<std::uint64_t>(*cpuSharedAddress));
	m_gpuHandle = *gpuSharedAddress;
}

std::uint8_t* CPUWriteAddressManager::GetCpuHandle(size_t frameIndex) const noexcept {
	return m_pCpuHandle + m_perFrameBufferSize * frameIndex;
}

D3D12_GPU_VIRTUAL_ADDRESS CPUWriteAddressManager::GetGpuHandle(size_t frameIndex) const noexcept {
	return m_gpuHandle + m_perFrameBufferSize * frameIndex;
}
