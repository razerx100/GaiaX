#ifndef __GRAPHICS_ENGINE_DX12_HPP__
#define __GRAPHICS_ENGINE_DX12_HPP__
#include <GraphicsEngine.hpp>
#include <D3DHeaders.hpp>
#include <string>
#include <IViewportAndScissorManager.hpp>
#include <memory>

class GraphicsEngineDx12 : public GraphicsEngine {
public:
	GraphicsEngineDx12(
		const char* appName,
		void* windowHandle, std::uint32_t width, std::uint32_t height,
		size_t bufferCount
	);
	~GraphicsEngineDx12() noexcept;

	void SetBackgroundColor(const Ceres::VectorF32& color) noexcept override;
	void SubmitModel(const IModel* const modelRef, bool texture = true) override;
	void Render() override;
	void Resize(std::uint32_t width, std::uint32_t height) override;
	void GetMonitorCoordinates(
		std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
	) override;
	void WaitForAsyncTasks() override;

	void SetShaderPath(const char* path) noexcept override;
	void InitResourceBasedObjects() override;
	void ProcessData() override;

public:
	static constexpr DXGI_FORMAT RENDER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

private:
	std::unique_ptr<IViewportAndScissorManager> m_viewportAndScissor;

	Ceres::VectorF32 m_backgroundColor;

	const std::string m_appName;
	std::string m_shaderPath;
};
#endif
