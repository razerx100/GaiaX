#include <GaiaInstance.hpp>
#include <RendererDx12.hpp>

Renderer* CreateGaiaInstance(
	const char* appName,
	void* windowHandle,
	std::uint32_t width, std::uint32_t height,
	ThreadPool& threadPool,
	RenderEngineType engineType, std::uint32_t bufferCount
) {
	return new RendererDx12(
		appName,
		windowHandle, width, height, bufferCount,
		threadPool, engineType
	);
}
