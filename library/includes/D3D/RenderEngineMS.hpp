#ifndef RENDER_ENGINE_MS_HPP_
#define RENDER_ENGINE_MS_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

class RenderEngineMS : public RenderEngineCommon<ModelManagerMS, RenderEngineMS>
{
	friend class RenderEngineCommon<ModelManagerMS, RenderEngineMS>;

public:
	RenderEngineMS(
		const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
	) override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

private:
	[[nodiscard]]
	static ModelManagerMS GetModelManager(
		const DeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

	[[nodiscard]]
	ID3D12Fence* GenericCopyStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
	);
	[[nodiscard]]
	ID3D12Fence* DrawingStage(
		size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
	);

	void SetupPipelineStages();

	void SetGraphicsDescriptorBufferLayout();
	void SetGraphicsDescriptors();

private:
	// Graphics
	// CBV
	static constexpr size_t s_cameraCBVRegisterSlot = 1u;

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
