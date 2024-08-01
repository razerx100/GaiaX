#ifndef RENDERER_DX12_HPP_
#define RENDERER_DX12_HPP_
#include <Renderer.hpp>
#include <string>
#include <ObjectManager.hpp>
#include <ThreadPool.hpp>

class RendererDx12 final : public Renderer
{
public:
	RendererDx12(
		const char* appName,
		void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		std::shared_ptr<ThreadPool>&& threadPool, RenderEngineType engineType
	);

	void Resize(std::uint32_t width, std::uint32_t height) override;

	[[nodiscard]]
	Resolution GetFirstDisplayCoordinates() const override;

	void SetBackgroundColour(const std::array<float, 4>& colour) noexcept override;
	void SetShaderPath(const wchar_t* path) noexcept override;
	void AddPixelShader(const ShaderName& pixelShader) override;
	void ChangePixelShader(std::uint32_t modelBundleID, const ShaderName& pixelShader) override;

	void SetMeshIndex(std::uint32_t modelBundleID, std::uint32_t meshBundleID) override;

	[[nodiscard]]
	// The returned Index is the texture's ID. Not its index in the shader. It should be
	// used to remove or bind the texture.
	size_t AddTexture(
		std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
	) override;
	void UnbindTexture(size_t index) override;
	[[nodiscard]]
	// The returned index is the index of the texture in the shader.
	std::uint32_t BindTexture(size_t index) override;
	void RemoveTexture(size_t index) override;

	[[nodiscard]]
	std::uint32_t AddModel(std::shared_ptr<ModelVS>&& model, const ShaderName& pixelShader) override;
	[[nodiscard]]
	std::uint32_t AddModel(std::shared_ptr<ModelMS>&& model, const ShaderName& pixelShader) override;
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const ShaderName& pixelShader
	) override;
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const ShaderName& pixelShader
	) override;
	void RemoveModelBundle(std::uint32_t bundleID) noexcept override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;
	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle) override;
	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	size_t AddMaterial(std::shared_ptr<Material> material) override;
	[[nodiscard]]
	std::vector<size_t> AddMaterials(std::vector<std::shared_ptr<Material>>&& materials) override;
	void UpdateMaterial(size_t index) const noexcept override;
	void RemoveMaterial(size_t index) noexcept override;

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera>&& camera) noexcept override;
	void SetCamera(std::uint32_t index) noexcept override;
	void RemoveCamera(std::uint32_t index) noexcept override;

	void Render() override;
	void WaitForGPUToFinish() override;

private:
	const std::string m_appName;
	std::uint32_t m_width;
	std::uint32_t m_height;
	std::uint32_t m_bufferCount;
	ObjectManager m_objectManager;
};
#endif
