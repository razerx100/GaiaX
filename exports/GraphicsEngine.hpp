#ifndef __GRAPHICS_ENGINE_HPP__
#define __GRAPHICS_ENGINE_HPP__
#include <cstdint>
#include <SUtility.hpp>
#include <IModel.hpp>
#include <DirectXColors.h>

#ifdef BUILD_GAIAX
#define GAIAX_DLL __declspec(dllexport)
#else
#define GAIAX_DLL __declspec(dllimport)
#endif

class GAIAX_DLL GraphicsEngine {
public:
	virtual ~GraphicsEngine() = default;

	virtual void SetBackgroundColor(DirectX::XMVECTORF32 color) noexcept = 0;
	virtual void SubmitModels(const IModel* const models, std::uint32_t modelCount) = 0;
	virtual void Render() = 0;
	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
	virtual SRect GetMonitorCoordinates() = 0;
	virtual void WaitForAsyncTasks() = 0;
};

GAIAX_DLL GraphicsEngine* __cdecl GetGraphicsEngineInstance() noexcept;
GAIAX_DLL void __cdecl InitGraphicsEngineInstance(
	const char* appName,
	void* windowHandle,
	void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount = 2u
);
GAIAX_DLL void __cdecl CleanUpGraphicsEngineInstance() noexcept;

#endif
