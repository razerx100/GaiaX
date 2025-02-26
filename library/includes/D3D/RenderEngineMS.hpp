#ifndef RENDER_ENGINE_MS_HPP_
#define RENDER_ENGINE_MS_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

class RenderEngineMS : public
	RenderEngineCommon
	<
		MeshManagerMS,
		GraphicsPipelineMS,
		RenderEngineMS
	>
{
	friend class RenderEngineCommon
		<
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
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
	) override;

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

private:
	void ExecutePipelineStages(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue,
		ID3D12Fence* waitFence
	);

	[[nodiscard]]
	ID3D12Fence* GenericCopyStage(
		size_t frameIndex, UINT64& counterValue, ID3D12Fence* waitFence
	);
	void DrawingStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
	);

	void SetGraphicsDescriptorBufferLayout();
	void SetGraphicsDescriptors();

	void _updatePerFrame(UINT64 frameIndex) const noexcept
	{
		m_modelBuffers.Update(frameIndex);
	}

private:
	ModelManagerMS m_modelManager;
	ModelBuffers   m_modelBuffers;

public:
	RenderEngineMS(const RenderEngineMS&) = delete;
	RenderEngineMS& operator=(const RenderEngineMS&) = delete;

	RenderEngineMS(RenderEngineMS&& other) noexcept
		: RenderEngineCommon{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) },
		m_modelBuffers{ std::move(other.m_modelBuffers) }
	{}
	RenderEngineMS& operator=(RenderEngineMS&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));
		m_modelManager = std::move(other.m_modelManager);
		m_modelBuffers = std::move(other.m_modelBuffers);

		return *this;
	}
};
#endif
