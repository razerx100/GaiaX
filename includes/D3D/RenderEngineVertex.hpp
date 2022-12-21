#ifndef RENDER_ENGINE_VERTEX_
#define RENDER_ENGINE_VERTEX_
#include <RenderEngine.hpp>

class RenderEngineVertex final : public RenderEngine {
public:
	void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex, float* clearColour
	) override;
	void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) override;
	void Present(ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex) override;
	void ExecutePostRenderStage() override;
};
#endif
