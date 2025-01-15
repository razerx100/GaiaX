#ifndef EXTERNAL_BUFFER_HPP_
#define EXTERNAL_BUFFER_HPP_
#include <cstdint>

enum class ExternalBufferType
{
	GPUOnly,
	CPUVisible
};

class ExternalBuffer
{
public:
	virtual ~ExternalBuffer() = default;

	virtual void Create(size_t bufferSize) = 0;

	[[nodiscard]]
	virtual size_t BufferSize() const noexcept = 0;

	[[nodiscard]]
	virtual std::uint8_t* CPUHandle() const = 0;
};
#endif
