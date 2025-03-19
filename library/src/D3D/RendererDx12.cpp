#include <RendererDx12.hpp>

RendererDx12::RendererDx12(
	[[maybe_unused]] const char* appName,
	void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
	std::shared_ptr<ThreadPool>&& threadPool, RenderEngineType engineType
) : m_gaia{
		windowHandle, width, height, bufferCount, std::move(threadPool), engineType
	}
{}

void RendererDx12::FinaliseInitialisation()
{
	m_gaia.FinaliseInitialisation();
}

void RendererDx12::Render()
{
	m_gaia.Render();
}

void RendererDx12::Resize(std::uint32_t width, std::uint32_t height)
{
	m_gaia.Resize(width, height);
}

Renderer::Extent RendererDx12::GetCurrentRenderingExtent() const noexcept
{
	auto [width, height] = m_gaia.GetCurrentRenderArea();

	return Renderer::Extent{ .width = width, .height = height };
}

Renderer::Extent RendererDx12::GetFirstDisplayCoordinates() const
{
	auto [width, height] = m_gaia.GetFirstDisplayCoordinates();

	return Renderer::Extent{ .width = width, .height = height };
}

void RendererDx12::SetShaderPath(const wchar_t* path)
{
	m_gaia.GetRenderEngine().SetShaderPath(path);
}

std::uint32_t RendererDx12::AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline)
{
	return m_gaia.GetRenderEngine().AddGraphicsPipeline(gfxPipeline);
}

void RendererDx12::ChangeModelPipelineInBundle(
	std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
	std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
) {
	m_gaia.GetRenderEngine().ChangeModelPipelineInBundle(
		modelBundleIndex, modelIndex, oldPipelineIndex, newPipelineIndex
	);
}

void RendererDx12::RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept
{
	m_gaia.GetRenderEngine().RemoveGraphicsPipeline(pipelineIndex);
}

size_t RendererDx12::AddTexture(STexture&& texture)
{
	return m_gaia.GetRenderEngine().AddTexture(std::move(texture));
}

void RendererDx12::UnbindTexture(size_t textureIndex, std::uint32_t bindingIndex)
{
	m_gaia.GetRenderEngine().UnbindTexture(textureIndex, bindingIndex);
}

std::uint32_t RendererDx12::BindTexture(size_t textureIndex)
{
	return m_gaia.GetRenderEngine().BindTexture(textureIndex);
}

void RendererDx12::UnbindExternalTexture(std::uint32_t bindingIndex)
{
	m_gaia.GetRenderEngine().UnbindExternalTexture(bindingIndex);
}

void RendererDx12::RebindExternalTexture(size_t textureIndex, std::uint32_t bindingIndex)
{
	m_gaia.GetRenderEngine().RebindExternalTexture(textureIndex, bindingIndex);
}

std::uint32_t RendererDx12::BindExternalTexture(size_t textureIndex)
{
	return m_gaia.GetRenderEngine().BindExternalTexture(textureIndex);
}

void RendererDx12::RemoveTexture(size_t textureIndex)
{
	m_gaia.GetRenderEngine().RemoveTexture(textureIndex);
}

void RendererDx12::WaitForGPUToFinish()
{
	m_gaia.GetRenderEngine().WaitForGPUToFinish();
}

std::uint32_t RendererDx12::AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle)
{
	return m_gaia.GetRenderEngine().AddModelBundle(std::move(modelBundle));
}

void RendererDx12::RemoveModelBundle(std::uint32_t bundleIndex) noexcept
{
	m_gaia.GetRenderEngine().RemoveModelBundle(bundleIndex);
}

std::uint32_t RendererDx12::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
{
	return m_gaia.GetRenderEngine().AddMeshBundle(std::move(meshBundle));
}

void RendererDx12::RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
{
	m_gaia.GetRenderEngine().RemoveMeshBundle(bundleIndex);
}

std::uint32_t RendererDx12::AddCamera(std::shared_ptr<Camera>&& camera) noexcept
{
	return m_gaia.GetRenderEngine().AddCamera(std::move(camera));
}

void RendererDx12::SetCamera(std::uint32_t index) noexcept
{
	m_gaia.GetRenderEngine().SetCamera(index);
}

void RendererDx12::RemoveCamera(std::uint32_t index) noexcept
{
	m_gaia.GetRenderEngine().RemoveCamera(index);
}

ExternalResourceManager* RendererDx12::GetExternalResourceManager() noexcept
{
	return m_gaia.GetRenderEngine().GetExternalResourceManager();
}

void RendererDx12::UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails)
{
	m_gaia.GetRenderEngine().UpdateExternalBufferDescriptor(bindingDetails);
}

void RendererDx12::UploadExternalBufferGPUOnlyData(
	std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData,
	size_t srcDataSizeInBytes, size_t dstBufferOffset
) {
	m_gaia.GetRenderEngine().UploadExternalBufferGPUOnlyData(
		externalBufferIndex, std::move(cpuData), srcDataSizeInBytes, dstBufferOffset
	);
}

void RendererDx12::QueueExternalBufferGPUCopy(
	std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
	size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
) {
	m_gaia.GetRenderEngine().QueueExternalBufferGPUCopy(
		externalBufferSrcIndex, externalBufferDstIndex, dstBufferOffset, srcBufferOffset,
		srcDataSizeInBytes
	);
}

std::uint32_t RendererDx12::AddExternalRenderPass()
{
	return m_gaia.AddExternalRenderPass();
}

ExternalRenderPass* RendererDx12::GetExternalRenderPassRP(size_t index) const noexcept
{
	return m_gaia.GetRenderEngine().GetExternalRenderPassRP(index);
}

std::shared_ptr<ExternalRenderPass> RendererDx12::GetExternalRenderPassSP(
	size_t index
) const noexcept {
	return m_gaia.GetRenderEngine().GetExternalRenderPassSP(index);
}

void RendererDx12::SetSwapchainExternalRenderPass()
{
	m_gaia.SetSwapchainExternalRenderPass();
}

ExternalRenderPass* RendererDx12::GetSwapchainExternalRenderPassRP() const noexcept
{
	return m_gaia.GetRenderEngine().GetSwapchainExternalRenderPassRP();
}

std::shared_ptr<ExternalRenderPass> RendererDx12::GetSwapchainExternalRenderPassSP() const noexcept
{
	return m_gaia.GetRenderEngine().GetSwapchainExternalRenderPassSP();
}

void RendererDx12::RemoveExternalRenderPass(size_t index) noexcept
{
	m_gaia.GetRenderEngine().RemoveExternalRenderPass(index);
}

void RendererDx12::RemoveSwapchainExternalRenderPass() noexcept
{
	m_gaia.GetRenderEngine().RemoveSwapchainExternalRenderPass();
}

size_t RendererDx12::GetActiveRenderPassCount() const noexcept
{
	return m_gaia.GetRenderEngine().GetActiveRenderPassCount();
}

ExternalFormat RendererDx12::GetSwapchainFormat() const noexcept
{
	return m_gaia.GetSwapchainFormat();
}
