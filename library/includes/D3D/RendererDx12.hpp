#ifndef RENDERER_DX12_HPP_
#define RENDERER_DX12_HPP_
#include <Renderer.hpp>
#include <string>
#include <ObjectManager.hpp>
#include <ThreadPool.hpp>
#include <ISharedDataContainer.hpp>

class RendererDx12 final : public Renderer
{
public:
	RendererDx12(
		const char* appName,
		void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		ThreadPool& threadPool, ISharedDataContainer& sharedContainer, RenderEngineType engineType
	);

	void Resize(std::uint32_t width, std::uint32_t height) override;

	[[nodiscard]]
	Resolution GetFirstDisplayCoordinates() const override;

	void SetBackgroundColour(const std::array<float, 4>& colour) noexcept override;
	void SetShaderPath(const wchar_t* path) noexcept override;

	[[nodiscard]]
	size_t AddTexture(
		std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
	) override;

	void AddModelSet(
		std::vector<std::shared_ptr<Model>>&& models, const std::wstring& pixelShader
	) override;
	void AddMeshletModelSet(
		std::vector<MeshletModel>&& meshletModels, const std::wstring& pixelShader
	) override;
	void AddModelInputs(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
	) override;
	void AddModelInputs(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
		std::vector<std::uint32_t>&& gPrimIndices
	) override;

	void AddMaterial(std::shared_ptr<Material> material) override;
	void AddMaterials(std::vector<std::shared_ptr<Material>>&& materials) override;

	void Update() override;
	void Render() override;
	void WaitForAsyncTasks() override;
	void ProcessData() override;

private:
	const std::string m_appName;
	std::uint32_t m_width;
	std::uint32_t m_height;
	std::uint32_t m_bufferCount;
	ObjectManager m_objectManager;
};
#endif
