#ifndef CONSTANT_BUFFER_HPP_
#define CONSTANT_BUFFER_HPP_
#include <D3DHeaders.hpp>
#include <GaiaDataTypes.hpp>

class CPUConstantBuffer {
public:
	void Init(size_t bufferSize) noexcept;
	void SetMemoryAddresses() noexcept;

	[[nodiscard]]
	std::uint8_t* GetCpuHandle() const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuHandle() const noexcept;

private:
	SharedAddressPair m_sharedBufferAddresses;
	std::uint8_t* m_pCpuHandle;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuHandle;
};
#endif
