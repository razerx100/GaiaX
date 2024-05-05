#ifndef RENDERER_HPP_
#define RENDERER_HPP_
#include <cstdint>
#include <memory>
#include <array>
#include <string>
#include <RendererTypes.hpp>

#include <Model.hpp>
#include <Material.hpp>
#include <MeshBundle.hpp>
#include <Camera.hpp>

class Renderer
{
public:
	struct Resolution {
		std::uint64_t width;
		std::uint64_t height;
	};

	virtual ~Renderer() = default;

	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;

	[[nodiscard]]
	virtual Resolution GetFirstDisplayCoordinates() const = 0;

	virtual void SetBackgroundColour(const std::array<float, 4>& colour) noexcept = 0;
	virtual void SetShaderPath(const wchar_t* path) noexcept = 0;

	[[nodiscard]]
	virtual size_t AddTexture(
		std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
	) = 0; // Returns the index of the texture in its Resource Heap

	virtual void AddModel(std::shared_ptr<ModelVS>&& model, const std::wstring& pixelShader) = 0;
	virtual void AddModel(std::shared_ptr<ModelMS>&& model, const std::wstring& pixelShader) = 0;
	virtual void AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& pixelShader
	) = 0;
	virtual void AddModelBundle(
		std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const std::wstring& pixelShader
	) = 0;

	virtual void AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) = 0;
	virtual void AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle) = 0;

	virtual void AddMaterial(std::shared_ptr<Material> material) = 0;
	virtual void AddMaterials(std::vector<std::shared_ptr<Material>>&& materials) = 0;

	virtual void SetCamera(std::shared_ptr<Camera>&& camera) = 0;

	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void WaitForAsyncTasks() = 0;
	virtual void ProcessData() = 0;
};
#endif
