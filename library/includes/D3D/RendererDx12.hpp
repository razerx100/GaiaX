#ifndef RENDERER_DX12_HPP_
#define RENDERER_DX12_HPP_
#include <RendererCommonTypes.hpp>
#include <string>
#include <ThreadPool.hpp>
#include <Gaia.hpp>

namespace Gaia
{
template<class RenderEngine_t>
class RendererDx12
{
public:
	RendererDx12(
		void* windowHandle, std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		std::shared_ptr<ThreadPool>&& threadPool
	) : m_gaia{ windowHandle, width, height, bufferCount, std::move(threadPool) }
	{}

	void FinaliseInitialisation()
	{
		m_gaia.FinaliseInitialisation();
	}

	void Resize(std::uint32_t width, std::uint32_t height)
	{
		m_gaia.Resize(width, height);
	}

	[[nodiscard]]
	RendererType::Extent GetCurrentRenderingExtent() const noexcept
	{
		auto [width, height] = m_gaia.GetCurrentRenderArea();

		return RendererType::Extent{ .width = width, .height = height };
	}

	[[nodiscard]]
	RendererType::Extent GetFirstDisplayCoordinates() const
	{
		auto [width, height] = m_gaia.GetFirstDisplayCoordinates();

		return RendererType::Extent{ .width = width, .height = height };
	}

	void SetShaderPath(const wchar_t* path)
	{
		m_gaia.GetRenderEngine().SetShaderPath(path);
	}

	[[nodiscard]]
	std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline)
	{
		return m_gaia.GetRenderEngine().AddGraphicsPipeline(gfxPipeline);
	}

	void ReconfigureModelPipelinesInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t decreasedModelsPipelineIndex,
		std::uint32_t increasedModelsPipelineIndex
	) {
		m_gaia.GetRenderEngine().ReconfigureModelPipelinesInBundle(
			modelBundleIndex, decreasedModelsPipelineIndex, increasedModelsPipelineIndex
		);
	}

	void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept
	{
		m_gaia.GetRenderEngine().RemoveGraphicsPipeline(pipelineIndex);
	}

	[[nodiscard]]
	size_t AddTexture(STexture&& texture)
	{
		return m_gaia.GetRenderEngine().AddTexture(std::move(texture));
	}

	void UnbindTexture(size_t textureIndex, std::uint32_t bindingIndex)
	{
		m_gaia.GetRenderEngine().UnbindTexture(textureIndex, bindingIndex);
	}

	[[nodiscard]]
	std::uint32_t BindTexture(size_t textureIndex)
	{
		return m_gaia.GetRenderEngine().BindTexture(textureIndex);
	}

	void UnbindExternalTexture(std::uint32_t bindingIndex)
	{
		m_gaia.GetRenderEngine().UnbindExternalTexture(bindingIndex);
	}

	void RebindExternalTexture(size_t textureIndex, std::uint32_t bindingIndex)
	{
		m_gaia.GetRenderEngine().RebindExternalTexture(textureIndex, bindingIndex);
	}

	[[nodiscard]]
	std::uint32_t BindExternalTexture(size_t textureIndex)
	{
		return m_gaia.GetRenderEngine().BindExternalTexture(textureIndex);
	}

	void RemoveTexture(size_t textureIndex)
	{
		m_gaia.GetRenderEngine().RemoveTexture(textureIndex);
	}

	[[nodiscard]]
	std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle)
	{
		return m_gaia.GetRenderEngine().AddModelBundle(std::move(modelBundle));
	}

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept
	{
		m_gaia.GetRenderEngine().RemoveModelBundle(bundleIndex);
	}

	[[nodiscard]]
	std::uint32_t AddMeshBundle(MeshBundleTemporaryData&& meshBundle)
	{
		return m_gaia.GetRenderEngine().AddMeshBundle(std::move(meshBundle));
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
	{
		m_gaia.GetRenderEngine().RemoveMeshBundle(bundleIndex);
	}

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera>&& camera) noexcept
	{
		return m_gaia.GetRenderEngine().AddCamera(std::move(camera));
	}

	void SetCamera(std::uint32_t index) noexcept
	{
		m_gaia.GetRenderEngine().SetCamera(index);
	}

	void RemoveCamera(std::uint32_t index) noexcept
	{
		m_gaia.GetRenderEngine().RemoveCamera(index);
	}

	[[nodiscard]]
	size_t WaitForCurrentBackBuffer()
	{
		return m_gaia.WaitForCurrentBackBuffer();
	}

	void Update() const noexcept { m_gaia.Update(); }
	void Render() { m_gaia.Render(); }

	void WaitForGPUToFinish()
	{
		m_gaia.GetRenderEngine().WaitForGPUToFinish();
	}

public:
	// External stuff
	[[nodiscard]]
	auto&& GetExternalResourceManager(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(
			self.m_gaia.GetRenderEngine().GetExternalResourceManager()
		);
	}

	void UpdateExternalBufferDescriptor(
		const ExternalBufferBindingDetails& bindingDetails
	) {
		m_gaia.GetRenderEngine().UpdateExternalBufferDescriptor(bindingDetails);
	}

	void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData,
		size_t srcDataSizeInBytes, size_t dstBufferOffset
	) {
		m_gaia.GetRenderEngine().UploadExternalBufferGPUOnlyData(
			externalBufferIndex, std::move(cpuData), srcDataSizeInBytes, dstBufferOffset
		);
	}

	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset = 0, size_t srcDataSizeInBytes = 0
	) {
		m_gaia.GetRenderEngine().QueueExternalBufferGPUCopy(
			externalBufferSrcIndex, externalBufferDstIndex, dstBufferOffset, srcBufferOffset,
			srcDataSizeInBytes
		);
	}

	void AddLocalPipelinesInExternalRenderPass(
		std::uint32_t modelBundleIndex, size_t renderPassIndex
	) {
		m_gaia.GetRenderEngine().AddLocalPipelinesInExternalRenderPass(
			modelBundleIndex, renderPassIndex
		);
	}

	[[nodiscard]]
	std::uint32_t AddExternalRenderPass()
	{
		return m_gaia.AddExternalRenderPass();
	}

	[[nodiscard]]
	std::shared_ptr<D3DExternalRenderPass> GetExternalRenderPassSP(
		size_t index
	) const noexcept {
		return m_gaia.GetRenderEngine().GetExternalRenderPassSP(index);
	}

	void SetSwapchainExternalRenderPass()
	{
		m_gaia.SetSwapchainExternalRenderPass();
	}

	[[nodiscard]]
	std::shared_ptr<D3DExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept
	{
		return m_gaia.GetRenderEngine().GetSwapchainExternalRenderPassSP();
	}

	void RemoveExternalRenderPass(size_t index) noexcept
	{
		m_gaia.GetRenderEngine().RemoveExternalRenderPass(index);
	}

	void RemoveSwapchainExternalRenderPass() noexcept
	{
		m_gaia.GetRenderEngine().RemoveSwapchainExternalRenderPass();
	}

	[[nodiscard]]
	size_t GetActiveRenderPassCount() const noexcept
	{
		return m_gaia.GetRenderEngine().GetActiveRenderPassCount();
	}

	[[nodiscard]]
	ExternalFormat GetSwapchainFormat() const noexcept
	{
		return m_gaia.GetSwapchainFormat();
	}

private:
	Gaia<RenderEngine_t> m_gaia;

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
}
#endif
