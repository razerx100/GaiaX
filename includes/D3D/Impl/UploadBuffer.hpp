#ifndef __UPLOAD_BUFFER_HPP__
#define __UPLOAD_BUFFER_HPP__
#include <D3DHeaders.hpp>
#include <cstdint>

class UploadBuffer {
public:
	void CreateBuffer(ID3D12Device* device, std::uint64_t bufferSize);

	std::uint8_t* GetCPUHandle() const noexcept;
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUHandle() const noexcept;

private:
	ComPtr<ID3D12Resource> m_buffer;
	std::uint8_t* m_cpuHandle;
};
#endif
