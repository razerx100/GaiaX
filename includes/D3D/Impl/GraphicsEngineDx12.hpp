#ifndef __GRAPHICS_ENGINE_DX12_HPP__
#define __GRAPHICS_ENGINE_DX12_HPP__
#include <GraphicsEngine.hpp>
#include <D3DHeaders.hpp>
#include <string>

class GraphicsEngineDx12 : public GraphicsEngine {
public:
	GraphicsEngineDx12(
		const char* appName,
		void* windowHandle, std::uint32_t width, std::uint32_t height,
		size_t bufferCount
	);
	~GraphicsEngineDx12() noexcept;

	[[nodiscard]]
	size_t RegisterResource(const void* data, size_t size) override;

	void SetBackgroundColor(const Ceres::Float32_4& color) noexcept override;
	void SubmitModel(const IModel* const modelRef) override;
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
	Ceres::Float32_4 m_backgroundColor;

	const std::string m_appName;
	std::string m_shaderPath;
};
#endif
