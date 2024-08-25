#include <GaiaInstance.hpp>
#include <RendererDx12.hpp>

std::shared_ptr<Renderer> CreateGaiaInstance(
	const char* appName,
	void* windowHandle,
	std::uint32_t width, std::uint32_t height, std::shared_ptr<ThreadPool> threadPool,
	RenderEngineType engineType, std::uint32_t bufferCount
) {
	return std::make_shared<RendererDx12>(
		appName, windowHandle, width, height, bufferCount, std::move(threadPool), engineType
	);
}
