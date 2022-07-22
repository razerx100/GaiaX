#ifndef CONSTANT_BUFFER_HPP_
#define CONSTANT_BUFFER_HPP_
#include <D3DHeaders.hpp>
#include <GaiaDataTypes.hpp>

class CPUConstantBuffer {
public:
	CPUConstantBuffer() noexcept;

	void Init(size_t bufferSize, std::uint32_t frameCount = 1u) noexcept;
	void SetMemoryAddresses() noexcept;

	[[nodiscard]]
	std::uint8_t* GetCpuHandle(size_t frameIndex = 0u) const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuHandle(size_t frameIndex = 0u) const noexcept;

private:
	SharedAddressPair m_sharedBufferAddresses;
	std::uint8_t* m_pCpuHandle;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuHandle;
	std::size_t m_perFrameBufferSize;
};
#endif
