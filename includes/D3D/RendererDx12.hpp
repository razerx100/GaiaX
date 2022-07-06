#ifndef RENDERER_DX12_HPP_
#define RENDERER_DX12_HPP_
#include <Renderer.hpp>
#include <string>

class RendererDx12 final : public Renderer {
public:
	RendererDx12(
		const char* appName,
		void* windowHandle, std::uint32_t width, std::uint32_t height,
		std::uint32_t bufferCount
	);
	~RendererDx12() noexcept override;

	void Resize(std::uint32_t width, std::uint32_t height) override;

	[[nodiscard]]
	virtual Resolution GetDisplayCoordinates(std::uint32_t displayIndex = 0u) const override;

	[[nodiscard]]
	size_t RegisterResource(
		std::unique_ptr<std::uint8_t> textureData,
		size_t width, size_t height, bool components16bits = false
	) override;

	void SetThreadPool(std::shared_ptr<IThreadPool> threadPoolArg) noexcept override;
	void SetBackgroundColour(const std::array<float, 4>& colour) noexcept override;
	void SubmitModels(
		std::vector<std::shared_ptr<IModel>>&& models, std::unique_ptr<IModelInputs> modelInputs
	) override;
	void Render() override;
	void WaitForAsyncTasks() override;

	void SetShaderPath(const char* path) noexcept override;
	void InitResourceBasedObjects() override;
	void ProcessData() override;

	void SetSharedDataContainer(
		std::shared_ptr<ISharedDataContainer> sharedData
	) noexcept override;

private:
	std::array<float, 4> m_backgroundColour;

	const std::string m_appName;
	std::string m_shaderPath;
	std::uint32_t m_width;
	std::uint32_t m_height;
};
#endif
