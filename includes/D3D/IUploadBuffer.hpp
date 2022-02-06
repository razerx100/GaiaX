#ifndef __I_UPLOAD_BUFFER_HPP__
#define __I_UPLOAD_BUFFER_HPP__
#include <cstdint>
#include <D3DHeaders.hpp>
#include <D3DBuffer.hpp>
#include <memory>

class IUploadBuffer {
public:
	virtual ~IUploadBuffer() = default;

	virtual void MapBuffer() = 0;
	virtual std::uint8_t* GetCPUHandle() const noexcept = 0;
	virtual std::shared_ptr<D3DBuffer> GetBuffer() const noexcept = 0;
};

#endif
