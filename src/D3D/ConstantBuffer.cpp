#include <ConstantBuffer.hpp>
#include <Gaia.hpp>

CPUConstantBuffer::CPUConstantBuffer() noexcept
	: m_pCpuHandle{ nullptr }, m_gpuHandle{}, m_perFrameBufferSize{ 0 } {}

void CPUConstantBuffer::Init(size_t bufferSize, std::uint32_t frameCount) noexcept {
	m_perFrameBufferSize = bufferSize;
	m_sharedBufferAddresses = Gaia::constantBuffer->GetSharedAddresses(bufferSize * frameCount);
}

void CPUConstantBuffer::SetMemoryAddresses() noexcept {
	auto& [cpuSharedAddress, gpuSharedAddress] = m_sharedBufferAddresses;

	m_pCpuHandle =
		reinterpret_cast<std::uint8_t*>(static_cast<std::uint64_t>(*cpuSharedAddress));
	m_gpuHandle = *gpuSharedAddress;
}

std::uint8_t* CPUConstantBuffer::GetCpuHandle(size_t frameIndex) const noexcept {
	return m_pCpuHandle + m_perFrameBufferSize * frameIndex;
}

D3D12_GPU_VIRTUAL_ADDRESS CPUConstantBuffer::GetGpuHandle(size_t frameIndex) const noexcept {
	return m_gpuHandle + m_perFrameBufferSize * frameIndex;
}

