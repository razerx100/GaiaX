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
	std::shared_ptr<D3DBuffer> GetBuffer() const noexcept;

private:
	std::shared_ptr<D3DBuffer> m_uploadBuffer;
	std::uint8_t* m_cpuHandle;
};
#endif
