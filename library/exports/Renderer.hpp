#ifndef RENDERER_HPP_
#define RENDERER_HPP_
#include <cstdint>
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <RendererTypes.hpp>
#include <Shader.hpp>
#include <Texture.hpp>

#include <Model.hpp>
#include <Material.hpp>
#include <MeshBundle.hpp>
#include <Camera.hpp>
#include <ExternalResourceFactory.hpp>

class Renderer
{
public:
	struct Resolution
	{
		std::uint32_t width;
		std::uint32_t height;
	};

	virtual ~Renderer() = default;

	virtual void FinaliseInitialisation() = 0;

	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;

	[[nodiscard]]
	virtual Resolution GetFirstDisplayCoordinates() const = 0;

	virtual void SetBackgroundColour(const std::array<float, 4>& colour) noexcept = 0;
	virtual void SetShaderPath(const wchar_t* path) = 0;
	virtual void AddPixelShader(const ShaderName& pixelShader) = 0;
	virtual void ChangePixelShader(std::uint32_t modelBundleID, const ShaderName& pixelShader) = 0;

	[[nodiscard]]
	// The returned Index is the texture's ID. Not its index in the shader. It should be
	// used to remove or bind the texture.
	virtual size_t AddTexture(STexture&& texture) = 0;
	virtual void UnbindTexture(size_t index) = 0;
	[[nodiscard]]
	// The returned index is the index of the texture in the shader.
	virtual std::uint32_t BindTexture(size_t index) = 0;
	virtual void RemoveTexture(size_t index) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
	) = 0;
	virtual void RemoveModelBundle(std::uint32_t bundleID) noexcept = 0;

	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) = 0;
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
	[[nodiscard]]
	virtual ExternalResourceFactory* GetExternalResourceFactory() noexcept = 0;

	virtual void Render() = 0;
	virtual void WaitForGPUToFinish() = 0;
};
#endif
