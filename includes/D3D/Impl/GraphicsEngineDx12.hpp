#ifndef __GRAPHICS_ENGINE_DX12_HPP__
#define __GRAPHICS_ENGINE_DX12_HPP__
#include <IGraphicsEngine.hpp>
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

	void Resize(std::uint32_t width, std::uint32_t height) override;
	void GetMonitorCoordinates(
		std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
	) override;

	[[nodiscard]]
	size_t RegisterResource(
		const void* data,
		size_t width, size_t height, size_t pixelSizeInBytes
	) override;

	void SetBackgroundColour(const Ceres::Float32_4& colour) noexcept override;
	void SubmitModel(const IModel* const modelRef) override;
	void Render() override;
	void WaitForAsyncTasks() override;

	void SetShaderPath(const char* path) noexcept override;
	void InitResourceBasedObjects() override;
	void ProcessData() override;

public:
	static constexpr DXGI_FORMAT RENDER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT TEXTURE_FORMAT = DXGI_FORMAT_R32G32B32A32_FLOAT;

private:
	Ceres::Float32_4 m_backgroundColour;

	const std::string m_appName;
	std::string m_shaderPath;
};
#endif
