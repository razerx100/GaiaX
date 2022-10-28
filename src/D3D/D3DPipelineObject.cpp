#include <D3DPipelineObject.hpp>
#include <VertexLayout.hpp>
#include <d3dx12.h>
#include <Gaia.hpp>

void D3DPipelineObject::CreateGFXPipelineState(
	ID3D12Device* device, const VertexLayout& vertexLayout,
	ID3D12RootSignature* gfxRootSignature, const D3D12_SHADER_BYTECODE& vertexShader,
	const D3D12_SHADER_BYTECODE& pixelShader
) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1u;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1u;
	psoDesc.InputLayout = vertexLayout.GetLayoutDesc();
	psoDesc.pRootSignature = gfxRootSignature;
	psoDesc.VS = vertexShader;
	psoDesc.PS = pixelShader;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState));
}

void D3DPipelineObject::CreateComputePipelineState(
	ID3D12Device* device, ID3D12RootSignature* computeRootSignature,
	const D3D12_SHADER_BYTECODE& computeShader
) {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = computeRootSignature;
	psoDesc.CS = computeShader;

	device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState));
}

ID3D12PipelineState* D3DPipelineObject::Get() const noexcept {
	return m_pPipelineState.Get();
}
