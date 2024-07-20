#ifndef RENDERER_HPP_
#define RENDERER_HPP_
#include <cstdint>
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <RendererTypes.hpp>

#include <Model.hpp>
#include <Material.hpp>
#include <MeshBundle.hpp>
#include <Camera.hpp>
#include <Shader.hpp>

class Renderer
{
public:
	struct Resolution
	{
		std::uint64_t width;
		std::uint64_t height;
	};

	virtual ~Renderer() = default;

	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;

	[[nodiscard]]
	virtual Resolution GetFirstDisplayCoordinates() const = 0;

	virtual void SetBackgroundColour(const std::array<float, 4>& colour) noexcept = 0;
	virtual void SetShaderPath(const wchar_t* path) noexcept = 0;
	virtual void AddPixelShader(const ShaderName& pixelShader) = 0;
	virtual void ChangePixelShader(std::uint32_t modelBundleID, const ShaderName& pixelShader) = 0;

	[[nodiscard]]
	virtual size_t AddTexture(
		std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
	) = 0;
	virtual void UnbindTexture(size_t index) = 0;
	virtual void BindTexture(size_t index) = 0;
	virtual void RemoveTexture(size_t index) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddModel(
		std::shared_ptr<ModelVS>&& model, const ShaderName& pixelShader
	) = 0;
	[[nodiscard]]
	virtual std::uint32_t AddModel(std::shared_ptr<ModelMS>&& model, const ShaderName& pixelShader) = 0;
	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const ShaderName& pixelShader
	) = 0;
	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const ShaderName& pixelShader
	) = 0;
	virtual void RemoveModelBundle(std::uint32_t bundleID) noexcept = 0;

	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) = 0;
	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle) = 0;
	virtual void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept = 0;

	[[nodiscard]]
	virtual size_t AddMaterial(std::shared_ptr<Material> material) = 0;
	[[nodiscard]]
	virtual std::vector<size_t> AddMaterials(std::vector<std::shared_ptr<Material>>&& materials) = 0;
	virtual void UpdateMaterial(size_t index) const noexcept = 0;
	virtual void RemoveMaterial(size_t index) noexcept = 0;

	[[nodiscard]]
	virtual std::uint32_t AddCamera(std::shared_ptr<Camera>&& camera) noexcept = 0;
	virtual void SetCamera(std::uint32_t index) noexcept = 0;
	virtual void RemoveCamera(std::uint32_t index) noexcept = 0;

	virtual void Render() = 0;
	virtual void WaitForGPUToFinish() = 0;
};
#endif
