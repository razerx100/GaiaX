#ifndef __I_INDEX_BUFFER_HPP__
#define __I_INDEX_BUFFER_HPP__
#include <D3DHeaders.hpp>
#include <cstdint>

class IIndexBuffer {
public:
	virtual ~IIndexBuffer() = default;

	virtual D3D12_INDEX_BUFFER_VIEW* GetIndexBufferRef() noexcept = 0;
	virtual std::uint64_t GetIndexCount() const noexcept = 0;
};
#endif
