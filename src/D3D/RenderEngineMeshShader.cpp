#include <RenderEngineMeshShader.hpp>
#include <Gaia.hpp>

void RenderEngineMeshShader::ExecuteRenderStage(size_t frameIndex) {
	ID3D12GraphicsCommandList6* graphicsCommandList = Gaia::graphicsCmdList->GetCommandList6();

	ExecutePreGraphicsStage(graphicsCommandList, frameIndex);
	RecordDrawCommands(graphicsCommandList, frameIndex);
}

void RenderEngineMeshShader::RecordDrawCommands(
	ID3D12GraphicsCommandList6* graphicsCommandList, size_t frameIndex
) {

}
