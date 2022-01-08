#ifndef __UPLOAD_BUFFER_HPP__
#define __UPLOAD_BUFFER_HPP__
#include <IUploadBuffer.hpp>

class UploadBuffer : public IUploadBuffer {
public:
	void CreateBuffer(ID3D12Device* device, size_t bufferSize) override;

	std::uint8_t* GetCPUHandle() const noexcept override;
	size_t GetBufferSize() const noexcept override;
	ID3D12Resource* GetBuffer() const noexcept override;

private:
	ComPtr<ID3D12Resource> m_uploadBuffer;
	size_t m_bufferSize;
	std::uint8_t* m_cpuHandle;
};
#endif
