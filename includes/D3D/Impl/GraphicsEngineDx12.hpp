#ifndef __GRAPHICS_ENGINE_DX12_HPP__
#define __GRAPHICS_ENGINE_DX12_HPP__
#include <GraphicsEngine.hpp>
#include <D3DHeaders.hpp>
#include <cstdint>
#include <string>

class GraphicsEngineDx12 : public GraphicsEngine {
public:
	GraphicsEngineDx12(
		const char* appName,
		void* windowHandle, std::uint32_t width, std::uint32_t height,
		std::uint8_t bufferCount
	);
	~GraphicsEngineDx12() noexcept;

	void SetBackgroundColor(Color color) noexcept override;
	void SubmitCommands() override;
	void Render() override;
	void Resize(std::uint32_t width, std::uint32_t height) override;
	SRect GetMonitorCoordinates() override;
	void WaitForAsyncTasks() override;

	static constexpr DXGI_FORMAT RENDER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

private:
	void InitViewPortAndScissor(std::uint32_t width, std::uint32_t height) noexcept;

private:
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	Color m_backgroundColor;

	const std::string m_appName;
};
#endif
