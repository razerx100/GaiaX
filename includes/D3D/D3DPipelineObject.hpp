#ifndef D3D_PIPELINE_OBJECT_HPP_
#define D3D_PIPELINE_OBJECT_HPP_
#include <D3DHeaders.hpp>
#include <d3dx12.h>

class D3DPipelineObject {
public:
	void CreateGFXPipelineStateVertex(
		ID3D12Device2* device, const D3D12_INPUT_LAYOUT_DESC& vertexLayout,
		ID3D12RootSignature* gfxRootSignature, const D3D12_SHADER_BYTECODE& vertexShader,
		const D3D12_SHADER_BYTECODE& pixelShader
	);
	void CreateComputePipelineState(
		ID3D12Device2* device, ID3D12RootSignature* computeRootSignature,
		const D3D12_SHADER_BYTECODE& computeShader
	);
	void CreateGFXPipelineStateMesh(
		ID3D12Device2* device, ID3D12RootSignature* gfxRootSignature,
		const D3D12_SHADER_BYTECODE& meshShader, const D3D12_SHADER_BYTECODE& pixelShader
	);

	[[nodiscard]]
	ID3D12PipelineState* Get() const noexcept;

private:
	void CreatePipelineState(
		ID3D12Device2* device, struct CD3DX12_PIPELINE_STATE_STREAM1 subObjectDesc
	) noexcept;

	template<typename T>
	void PopulateRootSignature(T& psoDesc, ID3D12RootSignature* rootSignature) const noexcept {
		psoDesc.pRootSignature = rootSignature;
	}

	template<typename T>
	void PopulateGeneralGFXStates(
		T& psoDesc, ID3D12RootSignature* rootSignature, const D3D12_SHADER_BYTECODE& pixelShader
	) const noexcept {
		PopulateRootSignature(psoDesc, rootSignature);

		constexpr D3D12_DEPTH_STENCIL_DESC depthStencilState{
			.DepthEnable = TRUE,
			.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
			.StencilEnable = FALSE
		};

		constexpr DXGI_SAMPLE_DESC sampleDesc{
			.Count = 1u
		};

		psoDesc.PS = pixelShader;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = depthStencilState;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1u;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc = sampleDesc;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	}

private:
	ComPtr<ID3D12PipelineState> m_pPipelineState;
};
#endif
