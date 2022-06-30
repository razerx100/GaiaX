#ifndef PIPELINE_OBJECT_GFX_HPP_
#define PIPELINE_OBJECT_GFX_HPP_
#include <D3DHeaders.hpp>

class PipelineObjectGFX {
public:
	void CreatePipelineState(
		ID3D12Device* device,
		const D3D12_INPUT_LAYOUT_DESC& vertexLayout,
		ID3D12RootSignature* rootSignature,
		const D3D12_SHADER_BYTECODE& vertexShader,
		const D3D12_SHADER_BYTECODE& pixelShader
	);

	[[nodiscard]]
	ID3D12PipelineState* Get() const noexcept;

private:
	ComPtr<ID3D12PipelineState> m_pPipelineState;
};
#endif
