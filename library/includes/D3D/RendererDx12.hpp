#ifndef RENDERER_DX12_HPP_
#define RENDERER_DX12_HPP_
#include <Renderer.hpp>
#include <string>
#include <ThreadPool.hpp>
#include <Gaia.hpp>

class RendererDx12 final : public Renderer
{
public:
	RendererDx12(
		const char* appName,
		void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		std::shared_ptr<ThreadPool>&& threadPool, RenderEngineType engineType
	);

	void FinaliseInitialisation() override;

	void Resize(std::uint32_t width, std::uint32_t height) override;

	[[nodiscard]]
	Extent GetCurrentRenderingExtent() const noexcept override;

	[[nodiscard]]
	Extent GetFirstDisplayCoordinates() const override;

	void SetShaderPath(const wchar_t* path) override;

	[[nodiscard]]
	std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline) override;

	void ReconfigureModelPipelinesInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t decreasedModelsPipelineIndex,
		std::uint32_t increasedModelsPipelineIndex
	) override;

	void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept override;

	[[nodiscard]]
	size_t AddTexture(STexture&& texture) override;

	void UnbindTexture(size_t textureIndex, std::uint32_t bindingIndex) override;

	[[nodiscard]]
	std::uint32_t BindTexture(size_t textureIndex) override;

	void UnbindExternalTexture(std::uint32_t bindingIndex) override;

	void RebindExternalTexture(size_t textureIndex, std::uint32_t bindingIndex) override;

	[[nodiscard]]
	std::uint32_t BindExternalTexture(size_t textureIndex) override;

	void RemoveTexture(size_t textureIndex) override;

	[[nodiscard]]
	std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) override;

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera>&& camera) noexcept override;

	void SetCamera(std::uint32_t index) noexcept override;
	void RemoveCamera(std::uint32_t index) noexcept override;

	void Render() override;
	void WaitForGPUToFinish() override;

public:
	// External stuff
	[[nodiscard]]
	ExternalResourceManager* GetExternalResourceManager() noexcept override;

	void UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails) override;

	void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
		size_t dstBufferOffset
	) override;
	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset = 0, size_t srcDataSizeInBytes = 0
	) override;

	[[nodiscard]]
	std::uint32_t AddExternalRenderPass() override;
	[[nodiscard]]
	ExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept override;
	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetExternalRenderPassSP(
		size_t index
	) const noexcept override;

	void SetSwapchainExternalRenderPass() override;

	[[nodiscard]]
	ExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept override;
	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept override;

	void RemoveExternalRenderPass(size_t index) noexcept override;
	void RemoveSwapchainExternalRenderPass() noexcept override;

	[[nodiscard]]
	size_t GetActiveRenderPassCount() const noexcept override;

	[[nodiscard]]
	ExternalFormat GetSwapchainFormat() const noexcept override;

private:
	Gaia m_gaia;

public:
	RendererDx12(const RendererDx12&) = delete;
	RendererDx12& operator=(const RendererDx12&) = delete;

	RendererDx12(RendererDx12&& other) noexcept
		: m_gaia{ std::move(other.m_gaia) }
	{}
	RendererDx12& operator=(RendererDx12&& other) noexcept
	{
		m_gaia = std::move(other.m_gaia);

		return *this;
	}
};
#endif
