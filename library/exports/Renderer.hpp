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
#include <MeshBundle.hpp>
#include <Camera.hpp>
#include <ExternalResourceManager.hpp>
#include <ExternalRenderPass.hpp>

class Renderer
{
public:
	struct Extent
	{
		std::uint32_t width;
		std::uint32_t height;
	};

	virtual ~Renderer() = default;

	virtual void FinaliseInitialisation() = 0;

	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;

	[[nodiscard]]
	virtual Extent GetCurrentRenderingExtent() const noexcept = 0;

	[[nodiscard]]
	virtual Extent GetFirstDisplayCoordinates() const = 0;

	virtual void SetShaderPath(const wchar_t* path) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline) = 0;

	virtual void ChangeModelPipelineInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
		std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) = 0;
	virtual void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept = 0;

	// The returned Index is the texture resource's index. Not its bound index in the shader.
	// It should be used to remove or bind the texture.
	[[nodiscard]]
	virtual size_t AddTexture(STexture&& texture) = 0;

	virtual void UnbindTexture(size_t index) = 0;

	// The returned index is the index of the bound texture in the shader.
	[[nodiscard]]
	virtual std::uint32_t BindTexture(size_t index) = 0;

	virtual void RemoveTexture(size_t index) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) = 0;

	virtual void RemoveModelBundle(std::uint32_t bundleIndex) noexcept = 0;

	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) = 0;

	virtual void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept = 0;

	[[nodiscard]]
	virtual std::uint32_t AddCamera(std::shared_ptr<Camera>&& camera) noexcept = 0;

	virtual void SetCamera(std::uint32_t index) noexcept = 0;
	virtual void RemoveCamera(std::uint32_t index) noexcept = 0;

	virtual void Render() = 0;
	virtual void WaitForGPUToFinish() = 0;

public:
	// External stuff
	[[nodiscard]]
	virtual ExternalResourceManager* GetExternalResourceManager() noexcept = 0;

	// This function needs to be here since the External Resource manager doesn't store the
	// descriptor managers.
	virtual void UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails) = 0;

	// Must wait for the GPU to finish before uploading/copying something on the GPU.
	virtual void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
		size_t dstBufferOffset
	) = 0;
	virtual void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset = 0, size_t srcDataSizeInBytes = 0
	) = 0;

	// These can't be put in an independent manager because then that manager will need
	// to keep references of multiple other objects.

	// The resizing of the attachments must be externally managed.
	[[nodiscard]]
	virtual std::uint32_t AddExternalRenderPass() = 0;
	[[nodiscard]]
	virtual ExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept = 0;
	[[nodiscard]]
	virtual std::shared_ptr<ExternalRenderPass> GetExternalRenderPassSP(
		size_t index
	) const noexcept = 0;

	// This is should be almost identical to a normal RenderPass, the only difference would be that
	// it will copy a specified render target to the swapchain backbuffer at the end.
	virtual void SetSwapchainExternalRenderPass() = 0;

	[[nodiscard]]
	virtual ExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept = 0;
	[[nodiscard]]
	virtual std::shared_ptr<ExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept = 0;

	virtual void RemoveExternalRenderPass(size_t index) noexcept = 0;
	virtual void RemoveSwapchainExternalRenderPass() noexcept = 0;

	[[nodiscard]]
	virtual size_t GetActiveRenderPassCount() const noexcept = 0;

	// Necessary on Vulkan, as different devices can have different supported formats.
	[[nodiscard]]
	virtual ExternalFormat GetSwapchainFormat() const noexcept = 0;
};
#endif
