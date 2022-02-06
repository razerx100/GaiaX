#ifndef __UPLOAD_BUFFER_HPP__
#define __UPLOAD_BUFFER_HPP__
#include <IUploadBuffer.hpp>

class UploadBuffer : public IUploadBuffer {
public:
	UploadBuffer();

	void MapBuffer() override;
	std::uint8_t* GetCPUHandle() const noexcept override;
	std::shared_ptr<D3DBuffer> GetBuffer() const noexcept override;

private:
	std::shared_ptr<D3DBuffer> m_uploadBuffer;
	std::uint8_t* m_cpuHandle;
};
#endif
