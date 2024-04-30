#include <D3DPipelineObject.hpp>
#include <Gaia.hpp>

void D3DPipelineObject::CreateGFXPipelineStateVertex(
	ID3D12Device2* device, const D3D12_INPUT_LAYOUT_DESC& vertexLayout,
	ID3D12RootSignature* gfxRootSignature, const D3D12_SHADER_BYTECODE& vertexShader,
	const D3D12_SHADER_BYTECODE& pixelShader
) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{
		.VS = vertexShader,
		.InputLayout = vertexLayout
	};

	PopulateGeneralGFXStates(psoDesc, gfxRootSignature, pixelShader);

	CreatePipelineState(device, CD3DX12_PIPELINE_STATE_STREAM1(psoDesc));
}

void D3DPipelineObject::CreateComputePipelineState(
	ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
	const D3D12_SHADER_BYTECODE& computeShader
) {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{
		.CS = computeShader
	};

	PopulateRootSignature(psoDesc, computeRootSignature);

	CreatePipelineState(device, CD3DX12_PIPELINE_STATE_STREAM1(psoDesc));
}

ID3D12PipelineState* D3DPipelineObject::Get() const noexcept {
	return m_pPipelineState.Get();
}

void D3DPipelineObject::CreateGFXPipelineStateMesh(
	ID3D12Device2* device, ID3D12RootSignature* gfxRootSignature,
	const D3D12_SHADER_BYTECODE& meshShader, const D3D12_SHADER_BYTECODE& pixelShader
) {
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc{
		.MS = meshShader,
	};

	PopulateGeneralGFXStates(psoDesc, gfxRootSignature, pixelShader);

	CreatePipelineState(device, CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc));
}
