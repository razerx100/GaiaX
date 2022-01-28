#ifndef __PIPELINE_OBJECT_GFX_HPP__
#define __PIPELINE_OBJECT_GFX_HPP__
#include <IPipelineObject.hpp>
#include <memory>
#include <vector>

class PipelineObjectGFX : public IPipelineObject {
public:
	void CreatePipelineState(
		ID3D12Device* device,
		const class VertexLayout& vertexLayout,
		ID3D12RootSignature* rootSignature,
		const D3D12_SHADER_BYTECODE& vertexShader,
		const D3D12_SHADER_BYTECODE& pixelShader
	);

	ID3D12PipelineState* Get() const noexcept override;

private:
	ComPtr<ID3D12PipelineState> m_pPipelineState;
};
#endif
