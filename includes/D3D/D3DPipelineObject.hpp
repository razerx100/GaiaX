#ifndef D3D_PIPELINE_OBJECT_HPP_
#define D3D_PIPELINE_OBJECT_HPP_
#include <D3DHeaders.hpp>
#include <VertexLayout.hpp>

class D3DPipelineObject {
public:
	void CreateGFXPipelineState(
		ID3D12Device* device, const VertexLayout& vertexLayout,
		ID3D12RootSignature* gfxRootSignature, const D3D12_SHADER_BYTECODE& vertexShader,
		const D3D12_SHADER_BYTECODE& pixelShader
	);
	void CreateComputePipelineState(
		ID3D12Device* device, ID3D12RootSignature* computeRootSignature,
		const D3D12_SHADER_BYTECODE& computeShader
	);

	[[nodiscard]]
	ID3D12PipelineState* Get() const noexcept;

private:
	ComPtr<ID3D12PipelineState> m_pPipelineState;
};
#endif
