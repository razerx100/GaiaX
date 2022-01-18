#include <GaiaInstance.hpp>
#include <GraphicsEngineDx12.hpp>

GraphicsEngine* CreateGaiaInstance(
	const char* appName,
	void* windowHandle,
	std::uint32_t width, std::uint32_t height,
	size_t bufferCount
) {
	return new GraphicsEngineDx12(
		appName,
		windowHandle, width, height, bufferCount
	);
}
