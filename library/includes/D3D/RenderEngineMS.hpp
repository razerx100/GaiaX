#ifndef RENDER_ENGINE_MS_HPP_
#define RENDER_ENGINE_MS_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

class RenderEngineMS : public
	RenderEngineCommon
	<
		ModelManagerMS,
		MeshManagerMS,
		GraphicsPipelineMS,
		RenderEngineMS
	>
{
	friend class RenderEngineCommon
		<
			ModelManagerMS,
			MeshManagerMS,
			GraphicsPipelineMS,
			RenderEngineMS
		>;

public:
	RenderEngineMS(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	void FinaliseInitialisation(const DeviceManager& deviceManager) override;

	[[nodiscard]]
	std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) override;

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

private:
	void ExecutePipelineStages(
		size_t frameIndex, ID3D12Resource* swapchainBackBuffer, UINT64& counterValue,
		ID3D12Fence* waitFence
	);

	[[nodiscard]]
	ID3D12Fence* GenericCopyStage(
		size_t frameIndex, UINT64& counterValue, ID3D12Fence* waitFence
	);
	void DrawingStage(
		size_t frameIndex, ID3D12Resource* swapchainBackBuffer, UINT64& counterValue,
		ID3D12Fence* waitFence
	);

	void SetGraphicsDescriptorBufferLayout();
	void SetGraphicsDescriptors();

	void _updatePerFrame(UINT64 frameIndex) const noexcept
	{
		m_modelBuffers.Update(frameIndex);
	}

private:
	void DrawRenderPassPipelines(
		const D3DCommandList& graphicsCmdList, const ExternalRenderPass_t& renderPass
	) const noexcept;

public:
	RenderEngineMS(const RenderEngineMS&) = delete;
	RenderEngineMS& operator=(const RenderEngineMS&) = delete;

	RenderEngineMS(RenderEngineMS&& other) noexcept
		: RenderEngineCommon{ std::move(other) }
	{}
	RenderEngineMS& operator=(RenderEngineMS&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));

		return *this;
	}
};
#endif
