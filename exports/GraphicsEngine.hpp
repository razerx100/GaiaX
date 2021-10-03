#ifndef __GRAPHICS_ENGINE_HPP__
#define __GRAPHICS_ENGINE_HPP__
#include <cstdint>

#ifdef BUILD_GAIAX
#define GAIAX_DLL __declspec(dllexport)
#else
#define GAIAX_DLL __declspec(dllimport)
#endif

struct GAIAX_DLL SRect {
	long left;
	long top;
	long right;
	long bottom;
};

class GAIAX_DLL GraphicsEngine {
public:
	virtual ~GraphicsEngine() = default;

	virtual void SubmitCommands() = 0;
	virtual void Render() = 0;
	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
	virtual SRect GetMonitorCoordinates() = 0;
};

GAIAX_DLL GraphicsEngine* _cdecl GetGraphicsEngineInstance() noexcept;
GAIAX_DLL void _cdecl InitGraphicsEngineInstance(void* windowHandle);
GAIAX_DLL void _cdecl CleanUpGraphicsEngineInstance() noexcept;

#endif
