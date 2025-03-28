#ifndef EXTERNAL_BUFFER_HPP_
#define EXTERNAL_BUFFER_HPP_
#include <cstdint>
#include <array>
#include <ExternalFormat.hpp>

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

	virtual void Destroy() noexcept = 0;

	[[nodiscard]]
	virtual size_t BufferSize() const noexcept = 0;

	[[nodiscard]]
	virtual std::uint8_t* CPUHandle() const = 0;
};

enum class ExternalTexture2DType
{
	RenderTarget,
	Depth,
	Stencil
};

struct ExternalTextureCreationFlags
{
	// The two copy flags are only necessary on Vulkan for now. Until Dx12 Enhanced Barrier is more common.
	// It is available on Win11 only now.
	bool copySrc       = false;
	bool copyDst       = false;
	bool sampleTexture = false;
};

class ExternalTexture
{
public:
	struct Extent
	{
		std::uint32_t width;
		std::uint32_t height;
	};

public:
	virtual ~ExternalTexture() = default;

	virtual void Create(
		std::uint32_t width, std::uint32_t height, ExternalFormat format, ExternalTexture2DType type,
		const ExternalTextureCreationFlags& creationFlags
	) = 0;

	virtual void Destroy() noexcept = 0;

	[[nodiscard]]
	virtual Extent GetExtent() const noexcept = 0;
};
#endif
