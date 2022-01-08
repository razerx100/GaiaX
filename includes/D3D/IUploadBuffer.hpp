#ifndef __I_UPLOAD_BUFFER_HPP__
#define __I_UPLOAD_BUFFER_HPP__
#include <cstdint>
#include <D3DHeaders.hpp>

class IUploadBuffer {
public:
	virtual ~IUploadBuffer() = default;

	virtual void CreateBuffer(ID3D12Device* device, size_t bufferSize) = 0;
	virtual std::uint8_t* GetCPUHandle() const noexcept = 0;
	virtual size_t GetBufferSize() const noexcept = 0;
	virtual ID3D12Resource* GetBuffer() const noexcept = 0;
};

#endif
