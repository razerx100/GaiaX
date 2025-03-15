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

struct ExternalClearColour
{
	std::array<float, 4u> colour  = { 0.f, 0.f, 0.f, 0.f };
	float                 depth   = 1.f;
	std::uint32_t         stencil = 0u;
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

	// The two copy flags are only necessary on Vulkan for now.
	// And the clear colour is only necessary Dx12.
	virtual void Create(
		std::uint32_t width, std::uint32_t height, ExternalFormat format, ExternalTexture2DType type,
		bool copySrc, bool copyDst, const ExternalClearColour& clearColour = {}
	) = 0;

	virtual void Destroy() noexcept = 0;

	[[nodiscard]]
	virtual Extent GetExtent() const noexcept = 0;
};
#endif
