#ifndef UPLOAD_BUFFER_HPP_
#define UPLOAD_BUFFER_HPP_
#include <D3DBuffer.hpp>
#include <memory>

class UploadBuffer {
public:
	UploadBuffer();

	void MapBuffer();

	[[nodiscard]]
	std::uint8_t* GetCPUHandle() const noexcept;
	[[nodiscard]]
	D3DBufferShared GetBuffer() const noexcept;

private:
	D3DBufferShared m_uploadBuffer;
	std::uint8_t* m_cpuHandle;
};

using UploadBufferShared = std::shared_ptr<UploadBuffer>;
#endif
