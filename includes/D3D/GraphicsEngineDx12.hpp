#ifndef __GRAPHICS_ENGINE_DX12_HPP__
#define __GRAPHICS_ENGINE_DX12_HPP__
#include <GraphicsEngine.hpp>
#include <D3DHeaders.hpp>
#include <cstdint>

class GraphicsEngineDx12 : public GraphicsEngine {
public:
	GraphicsEngineDx12(void* windowHandle);
	~GraphicsEngineDx12();

	void SubmitCommands() override;
	void Render() override;
	void Resize(std::uint32_t width, std::uint32_t height) override;
	SRect GetMonitorCoordinates() override;

	static constexpr DXGI_FORMAT RENDER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
};
#endif
