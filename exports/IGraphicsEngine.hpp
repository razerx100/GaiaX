#ifndef __I_GRAPHICS_ENGINE_HPP__
#define __I_GRAPHICS_ENGINE_HPP__
#include <cstdint>

#ifdef BUILD_GAIAX
#define GAIAX_DLL __declspec(dllexport)
#else
#define GAIAX_DLL __declspec(dllimport)
#endif

namespace Ceres {
	struct VectorF32;
}

class GAIAX_DLL GraphicsEngine {
public:
	virtual ~GraphicsEngine() = default;

	virtual void SetBackgroundColor(const Ceres::VectorF32& color) noexcept = 0;
	virtual void SubmitModels(
		class IModel** models, std::uint32_t modelCount,
		bool texture = true
	) = 0;
	virtual void Render() = 0;
	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
	virtual void GetMonitorCoordinates(
		std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
	) = 0;
	virtual void WaitForAsyncTasks() = 0;

	virtual void SetShaderPath(const char* path) noexcept = 0;
	virtual void InitResourceBasedObjects() = 0;
};

#endif
