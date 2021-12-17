#ifndef __INDEX_BUFFER_HPP__
#define __INDEX_BUFFER_HPP__
#include <IIndexBuffer.hpp>
#include <vector>
#include <UploadBuffer.hpp>

class IndexBuffer : public IIndexBuffer {
public:
	IndexBuffer(ID3D12Device* device, const std::vector<std::uint16_t>& indices);

	D3D12_INDEX_BUFFER_VIEW* GetIndexBufferRef() noexcept override;
	std::uint64_t GetIndexCount() const noexcept override;

private:
	UploadBuffer m_buffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	std::uint64_t m_indexCount;
};
#endif
