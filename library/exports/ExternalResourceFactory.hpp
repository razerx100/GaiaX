#ifndef EXTERNAL_RESOURCE_FACTORY_HPP_
#define EXTERNAL_RESOURCE_FACTORY_HPP_
#include <memory>
#include <ExternalBuffer.hpp>

// Resource from here must be freed before the Renderer is destroyed.
class ExternalResourceFactory
{
public:
	virtual ~ExternalResourceFactory() = default;

	[[nodiscard]]
	// Return the index of the created buffer.
	virtual size_t CreateExternalBuffer(ExternalBufferType type) = 0;
	[[nodiscard]]
	// Return the index of the created Texture.
	virtual size_t CreateExternalTexture() = 0;

	[[nodiscard]]
	virtual ExternalBuffer* GetExternalBufferRP(size_t index) const noexcept = 0;
	[[nodiscard]]
	virtual ExternalTexture* GetExternalTextureRP(size_t index) const noexcept = 0;

	[[nodiscard]]
	virtual std::shared_ptr<ExternalBuffer> GetExternalBufferSP(size_t index) const noexcept = 0;
	[[nodiscard]]
	virtual std::shared_ptr<ExternalTexture> GetExternalTextureSP(size_t index) const noexcept = 0;

	// Thought about creating a Ring Buffer, but we won't be allocating new memory, so just
	// recreating the buffer objects wouldn't be that bad.
	virtual void RemoveExternalBuffer(size_t index) noexcept = 0;
	virtual void RemoveExternalTexture(size_t index) noexcept = 0;
};
#endif
