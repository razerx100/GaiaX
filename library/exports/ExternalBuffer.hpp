#ifndef EXTERNAL_BUFFER_HPP_
#define EXTERNAL_BUFFER_HPP_
#include <cstdint>
#include <memory>
#include <ExternalFormat.hpp>
#include <RendererCommonTypes.hpp>

// Uniform buffers need to be 16bytes aligned.
enum class ExternalBufferType
{
	GPUOnly,
	CPUVisibleUniform,
	CPUVisibleSSBO
};

template<class ExternalBufferImpl_t>
class ExternalBuffer
{
public:
	ExternalBuffer() : m_buffer{} {}
	ExternalBuffer(std::shared_ptr<ExternalBufferImpl_t> buffer) : m_buffer{ std::move(buffer) } {}

	void SetBufferImpl(std::shared_ptr<ExternalBufferImpl_t> buffer) noexcept
	{
		m_buffer = std::move(buffer);
	}

	void Create(size_t bufferSize) { m_buffer->Create(bufferSize); }

	void Destroy() noexcept { m_buffer->Destroy(); }

	[[nodiscard]]
	size_t BufferSize() const noexcept { return m_buffer->BufferSize(); }

	[[nodiscard]]
	std::uint8_t* CPUHandle() const { return m_buffer->CPUHandle(); }

private:
	std::shared_ptr<ExternalBufferImpl_t> m_buffer;

public:
	ExternalBuffer(const ExternalBuffer&) = delete;
	ExternalBuffer& operator=(const ExternalBuffer&) = delete;

	ExternalBuffer(ExternalBuffer&& other) noexcept
		: m_buffer{ std::move(other.m_buffer) }
	{}
	ExternalBuffer& operator=(ExternalBuffer&& other) noexcept
	{
		m_buffer = std::move(other.m_buffer);

		return *this;
	}
};

enum class ExternalTexture2DType
{
	RenderTarget,
	Depth,
	Stencil
};

struct ExternalTextureCreationFlags
{
	// The two copy flags are only necessary on Vulkan for now. Until Dx12 Enhanced Barrier is more
	// common. It is available on Win11 only now.
	bool copySrc       = false;
	bool copyDst       = false;
	bool sampleTexture = false;
};

template<class ExternalTextureImpl_t>
class ExternalTexture
{
public:
	ExternalTexture() : m_texture{} {}
	ExternalTexture(std::shared_ptr<ExternalTextureImpl_t> texture)
		: m_texture{ std::move(texture) }
	{}

	void SetTextureImpl(std::shared_ptr<ExternalTextureImpl_t> texture) noexcept
	{
		m_texture = std::move(texture);
	}

	void Create(
		std::uint32_t width, std::uint32_t height, ExternalFormat format,
		ExternalTexture2DType type, const ExternalTextureCreationFlags& creationFlags = {}
	) {
		m_texture->Create(width, height, format, type, creationFlags);
	}

	void Destroy() noexcept { m_texture->Destroy(); }

	[[nodiscard]]
	RendererType::Extent GetExtent() const noexcept { return m_texture->GetExtent(); }

private:
	std::shared_ptr<ExternalTextureImpl_t> m_texture;

public:
	ExternalTexture(const ExternalTexture&) = delete;
	ExternalTexture& operator=(const ExternalTexture&) = delete;

	ExternalTexture(ExternalTexture&& other) noexcept
		: m_texture{ std::move(other.m_texture) }
	{}
	ExternalTexture& operator=(ExternalTexture&& other) noexcept
	{
		m_texture = std::move(other.m_texture);

		return *this;
	}
};
#endif
