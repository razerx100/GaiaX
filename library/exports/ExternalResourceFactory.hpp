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
	virtual std::unique_ptr<ExternalBuffer> CreateExternalBuffer(ExternalBufferType type) const = 0;
};
#endif
