#ifndef __GPU_BUFFER_HPP__
#define __GPU_BUFFER_HPP__
#include <IGPUBuffer.hpp>

class GPUBuffer : public IGPUBuffer {
public:
	void CreateBuffer(ID3D12Device* device, size_t bufferSize) override;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUHandle() const noexcept override;
	size_t GetBufferSize() const noexcept override;
	ID3D12Resource* GetBuffer() const noexcept override;

private:
	ComPtr<ID3D12Resource> m_gpuBuffer;
	size_t m_bufferSize;
};
#endif
