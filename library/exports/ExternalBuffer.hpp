#ifndef EXTERNAL_BUFFER_HPP_
#define EXTERNAL_BUFFER_HPP_
#include <cstdint>

// Uniform buffers need to be 16bytes aligned.
enum class ExternalBufferType
{
	GPUOnly,
	CPUVisibleUniform,
	CPUVisibleSSBO
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
