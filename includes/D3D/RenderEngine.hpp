#ifndef RENDER_ENGINE_
#define RENDER_ENGINE_
#include <D3DHeaders.hpp>

class RenderEngine {
public:
	virtual ~RenderEngine() = default;

	virtual void ExecutePreRenderStage(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex, float* clearColour
	) = 0;
	virtual void RecordDrawCommands(
		ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
	) = 0;
	virtual void Present(ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex) = 0;
	virtual void ExecutePostRenderStage() = 0;
};
#endif
