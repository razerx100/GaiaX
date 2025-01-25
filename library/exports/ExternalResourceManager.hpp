#ifndef EXTERNAL_RESOURCE_MANAGER_HPP_
#define EXTERNAL_RESOURCE_MANAGER_HPP_
#include <memory>
#include <ExternalResourceFactory.hpp>
#include <GraphicsTechniqueExtension.hpp>

class ExternalResourceManager
{
public:
	virtual ~ExternalResourceManager() = default;

	// The extensions must be freed before the renderer is destroyed to free the resources.
	[[nodiscard]]
	virtual std::uint32_t AddGraphicsTechniqueExtension(
		std::shared_ptr<GraphicsTechniqueExtension> extension
	) = 0;

	virtual void RemoveGraphicsTechniqueExtension(std::uint32_t index) noexcept = 0;

	[[nodiscard]]
	virtual ExternalResourceFactory* GetResourceFactory() noexcept = 0;
	[[nodiscard]]
	virtual ExternalResourceFactory const* GetResourceFactory() const noexcept = 0;
};
#endif
