#ifndef __I_GPU_BUFFER_HPP__
#define __I_GPU_BUFFER_HPP__
#include <cstdint>
#include <D3DHeaders.hpp>

class IGPUBuffer {
public:
	virtual ~IGPUBuffer() = default;

	virtual void CreateBuffer(ID3D12Device* device, size_t bufferSize) = 0;
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUHandle() const noexcept = 0;
	virtual size_t GetBufferSize() const noexcept = 0;
	virtual ID3D12Resource* GetBuffer() const noexcept = 0;
};
#endif
