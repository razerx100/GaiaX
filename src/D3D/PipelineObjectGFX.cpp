#include <PipelineObjectGFX.hpp>
#include <IShader.hpp>
#include <VertexLayout.hpp>
#include <d3dx12.h>
#include <GraphicsEngineDx12.hpp>
#include <D3DThrowMacros.hpp>

void PipelineObjectGFX::CreatePipelineState(
	ID3D12Device* device,
	const VertexLayout& vertexLayout,
	ID3D12RootSignature* rootSignature,
	std::shared_ptr<IShader> vertexShader,
	std::shared_ptr<IShader> pixelShader
) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = GraphicsEngineDx12::RENDER_FORMAT;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.InputLayout = vertexLayout.GetLayout();
	psoDesc.pRootSignature = rootSignature;
	psoDesc.VS = vertexShader->GetByteCode();
	psoDesc.PS = pixelShader->GetByteCode();
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	HRESULT hr;
	D3D_THROW_FAILED(
		hr,
		device->CreateGraphicsPipelineState(
			&psoDesc,
			__uuidof(ID3D12PipelineState),
			&m_pPipelineState
		)
	);
}

ID3D12PipelineState* PipelineObjectGFX::Get() const noexcept {
	return m_pPipelineState.Get();
}
