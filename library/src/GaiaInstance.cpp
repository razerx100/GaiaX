#include <GaiaInstance.hpp>
#include <RendererDx12.hpp>

std::unique_ptr<Renderer> CreateGaiaInstance(
	const char* appName, void* windowHandle,
	std::uint32_t width, std::uint32_t height, std::shared_ptr<ThreadPool> threadPool,
	RenderEngineType engineType, std::uint32_t bufferCount
) {
	using namespace Gaia;

	if (engineType == RenderEngineType::MeshDraw)
		return std::make_unique<RendererDx12<RenderEngineMS>>(
			appName, windowHandle, width, height, bufferCount, std::move(threadPool)
		);
	else if (engineType == RenderEngineType::IndirectDraw)
		return std::make_unique<RendererDx12<RenderEngineVSIndirect>>(
			appName, windowHandle, width, height, bufferCount, std::move(threadPool)
		);
	else
		return std::make_unique<RendererDx12<RenderEngineVSIndividual>>(
			appName, windowHandle, width, height, bufferCount, std::move(threadPool)
		);
}
