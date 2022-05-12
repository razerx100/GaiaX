#ifndef PER_FRAME_BUFFERS_HPP_
#define PER_FRAME_BUFFERS_HPP_
#include <D3DHeaders.hpp>
#include <CPUAccessibleStorage.hpp>

class PerFrameBuffers {
public:
	PerFrameBuffers();

	void BindPerFrameBuffers(ID3D12GraphicsCommandList* graphicsCmdList) const noexcept;
	void SetMemoryAddresses() noexcept;

private:
	void InitBuffers();

private:
	SharedPair m_sharedMemoryHandles;
	std::uint8_t* m_pCpuHandle;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuHandle;
};
#endif