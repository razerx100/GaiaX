#ifndef D3D_RENDER_ENGINE_MS_HPP_
#define D3D_RENDER_ENGINE_MS_HPP_
#include <D3DRenderEngine.hpp>
#include <D3DModelManager.hpp>

namespace Gaia
{
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
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
	);

	void FinaliseInitialisation(const DeviceManager& deviceManager);

	[[nodiscard]]
	std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle);

	[[nodiscard]]
	std::uint32_t AddMeshBundle(MeshBundleTemporaryData&& meshBundle);

	void SetShaderPath(const std::wstring& shaderPath)
	{
		_setShaderPath(shaderPath);
	}

	void WaitForGPUToFinish()
	{
		WaitForGraphicsQueueToFinish();
	}

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

	[[nodiscard]]
	static ModelManagerMS CreateModelManager(
		[[maybe_unused]] ID3D12Device5* device,
		[[maybe_unused]] MemoryManager* memoryManager,
		[[maybe_unused]] std::uint32_t frameCount
	) {
		return ModelManagerMS{};
	}

private:
	void DrawRenderPassPipelines(
		const D3DCommandList& graphicsCmdList, const D3DExternalRenderPass& renderPass
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
}
#endif
