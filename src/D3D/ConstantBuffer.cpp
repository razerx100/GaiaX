#include <ConstantBuffer.hpp>
#include <Gaia.hpp>

void CPUConstantBuffer::Init(size_t bufferSize) noexcept {
	m_sharedBufferAddresses = Gaia::constantBuffer->GetSharedAddresses(bufferSize);
}

void CPUConstantBuffer::SetMemoryAddresses() noexcept {
	auto& [cpuSharedAddress, gpuSharedAddress] = m_sharedBufferAddresses;

	m_pCpuHandle =
		reinterpret_cast<std::uint8_t*>(static_cast<std::uint64_t>(*cpuSharedAddress));
	m_gpuHandle = *gpuSharedAddress;
}

std::uint8_t* CPUConstantBuffer::GetCpuHandle() const noexcept {
	return m_pCpuHandle;
}

D3D12_GPU_VIRTUAL_ADDRESS CPUConstantBuffer::GetGpuHandle() const noexcept {
	return m_gpuHandle;
}

