#ifndef D3D_PIPELINE_OBJECT_HPP_
#define D3D_PIPELINE_OBJECT_HPP_
#include <D3DHeaders.hpp>
#include <cassert>
#include <concepts>
#include <d3dx12.h>
#include <VertexLayout.hpp>

class StencilOpStateBuilder
{
public:
	StencilOpStateBuilder()
		: m_opState
		{
			.StencilFailOp      = D3D12_STENCIL_OP_KEEP,
			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
			.StencilPassOp      = D3D12_STENCIL_OP_KEEP,
			.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS
		}
	{}

	StencilOpStateBuilder& StencilOps(
		D3D12_STENCIL_OP failOp, D3D12_STENCIL_OP passOp, D3D12_STENCIL_OP depthFailOp
	) noexcept {
		m_opState.StencilFailOp      = failOp;
		m_opState.StencilPassOp      = passOp;
		m_opState.StencilDepthFailOp = depthFailOp;

		return *this;
	}

	StencilOpStateBuilder& CompareOp(D3D12_COMPARISON_FUNC compareOp) noexcept
	{
		m_opState.StencilFunc = compareOp;

		return *this;
	}

	[[nodiscard]]
	D3D12_DEPTH_STENCILOP_DESC Get() const noexcept { return m_opState; }

private:
	D3D12_DEPTH_STENCILOP_DESC m_opState;
};

class DepthStencilStateBuilder
{
public:
	DepthStencilStateBuilder()
		: m_depthStencilStateDesc{
			.DepthEnable      = FALSE,
			.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ZERO,
			.DepthFunc        = D3D12_COMPARISON_FUNC_LESS,
			.StencilEnable    = FALSE,
			.StencilReadMask  = D3D12_DEFAULT_STENCIL_READ_MASK,
			.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
			.FrontFace        = { StencilOpStateBuilder{}.Get() },
			.BackFace         = { StencilOpStateBuilder{}.Get() }
		}
	{}

	DepthStencilStateBuilder& Enable(
		BOOL depthTest, D3D12_DEPTH_WRITE_MASK depthWrite, BOOL stencilTest
	) noexcept {
		m_depthStencilStateDesc.DepthEnable      = depthTest;
		m_depthStencilStateDesc.DepthWriteMask   = depthWrite;
		m_depthStencilStateDesc.StencilEnable    = stencilTest;

		return *this;
	}

	DepthStencilStateBuilder& DepthCompareOp(D3D12_COMPARISON_FUNC compareOp) noexcept
	{
		m_depthStencilStateDesc.DepthFunc = compareOp;

		return *this;
	}

	DepthStencilStateBuilder& StencilOpStates(
		const StencilOpStateBuilder& front, const StencilOpStateBuilder& back
	) noexcept {
		m_depthStencilStateDesc.FrontFace = front.Get();
		m_depthStencilStateDesc.BackFace  = back.Get();

		return *this;
	}

	[[nodiscard]]
	D3D12_DEPTH_STENCIL_DESC Get() const noexcept { return m_depthStencilStateDesc; }

private:
	D3D12_DEPTH_STENCIL_DESC m_depthStencilStateDesc;
};

template<typename PipelineStateType>
class GraphicsPipelineBuilder
{
public:
	GraphicsPipelineBuilder(ID3D12RootSignature* rootSignature)
		: m_pipelineState{
			.pRootSignature        = rootSignature,
			.BlendState            = D3D12_BLEND_DESC
			{
				.AlphaToCoverageEnable  = FALSE,
				.IndependentBlendEnable = FALSE,
				.RenderTarget           = {}
			},
			.SampleMask            = UINT_MAX,
			.RasterizerState       = D3D12_RASTERIZER_DESC
			{
				.FillMode              = D3D12_FILL_MODE_SOLID,
				.CullMode              = D3D12_CULL_MODE_NONE,
				.FrontCounterClockwise = FALSE,
				.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS,
				.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
				.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
				.DepthClipEnable       = TRUE,
				.MultisampleEnable     = FALSE,
				.AntialiasedLineEnable = FALSE,
				.ForcedSampleCount     = 0,
				.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
			},
			.DepthStencilState     = DepthStencilStateBuilder{}.Get(),
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets      = 0u,
			.RTVFormats            = { DXGI_FORMAT_UNKNOWN },
			.DSVFormat             = DXGI_FORMAT_UNKNOWN,
			.SampleDesc            = DXGI_SAMPLE_DESC{ .Count = 1u, .Quality = 0u },
			.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE
		}, m_vertexLayout{}
	{}

	// Can only be set on a Vertex Shader based Pipeline.
	GraphicsPipelineBuilder& SetInputAssembler(
		const VertexLayout& vertexLayout
	) noexcept requires std::is_same_v<PipelineStateType, D3D12_GRAPHICS_PIPELINE_STATE_DESC> {
		m_vertexLayout              = vertexLayout;

		m_pipelineState.InputLayout = m_vertexLayout.GetLayoutDesc();

		return *this;
	}
	GraphicsPipelineBuilder& SetVertexStage(
		const D3D12_SHADER_BYTECODE& vertexShader, const D3D12_SHADER_BYTECODE& pixelShader
	) noexcept requires std::is_same_v<PipelineStateType, D3D12_GRAPHICS_PIPELINE_STATE_DESC> {
		m_pipelineState.VS = vertexShader;
		m_pipelineState.PS = pixelShader;

		return *this;
	}

	GraphicsPipelineBuilder& SetCullMode(D3D12_CULL_MODE cullMode) noexcept
	{
		m_pipelineState.RasterizerState.CullMode = cullMode;

		return *this;
	}

	GraphicsPipelineBuilder& AddRenderTarget(
		DXGI_FORMAT rtvFormat, const D3D12_RENDER_TARGET_BLEND_DESC& blendState
	) {
		assert(
			m_pipelineState.NumRenderTargets <= std::size(m_pipelineState.RTVFormats)
			&& "Can't have more than 8 RTVs attached to a pipeline."
		);

		const size_t currentRTVIndex = m_pipelineState.NumRenderTargets;

		m_pipelineState.BlendState.RenderTarget[currentRTVIndex] = blendState;
		m_pipelineState.RTVFormats[currentRTVIndex]              = rtvFormat;

		++m_pipelineState.NumRenderTargets;

		return *this;
	}

	GraphicsPipelineBuilder& SetDepthStencilState(
		const DepthStencilStateBuilder& depthStencilBuilder, DXGI_FORMAT dsvFormat
	) noexcept {
		m_pipelineState.DepthStencilState = depthStencilBuilder.Get();
		m_pipelineState.DSVFormat         = dsvFormat;

		return *this;
	}

	// Can only be set on a Mesh Shader based Pipeline.
	GraphicsPipelineBuilder& SetMeshStage(
		const D3D12_SHADER_BYTECODE& meshShader, const D3D12_SHADER_BYTECODE& pixelShader
	) noexcept requires std::is_same_v<PipelineStateType, D3DX12_MESH_SHADER_PIPELINE_STATE_DESC> {
		m_pipelineState.MS = meshShader;
		m_pipelineState.PS = pixelShader;

		return *this;
	}
	GraphicsPipelineBuilder& SetAmplificationStage(
		const D3D12_SHADER_BYTECODE& amplificationShader
	) noexcept requires std::is_same_v<PipelineStateType, D3DX12_MESH_SHADER_PIPELINE_STATE_DESC> {
		m_pipelineState.AS = amplificationShader;

		return *this;
	}

	[[nodiscard]]
	PipelineStateType Get() const noexcept { return m_pipelineState; }

private:
	PipelineStateType m_pipelineState;
	VertexLayout      m_vertexLayout;

public:
	GraphicsPipelineBuilder(const GraphicsPipelineBuilder& other) noexcept
		: m_pipelineState{ other.m_pipelineState },
		m_vertexLayout{ other.m_vertexLayout }
	{
		// GetLayoutDesc gets a new object with the updated pointers.
		m_pipelineState.InputLayout = m_vertexLayout.GetLayoutDesc();
	}
	GraphicsPipelineBuilder& operator=(const GraphicsPipelineBuilder& other) noexcept
	{
		m_pipelineState = other.m_pipelineState;
		m_vertexLayout  = other.m_vertexLayout;

		// GetLayoutDesc gets a new object with the updated pointers.
		m_pipelineState.InputLayout = m_vertexLayout.GetLayoutDesc();

		return *this;
	}

	GraphicsPipelineBuilder(GraphicsPipelineBuilder&& other) noexcept
		: m_pipelineState{ other.m_pipelineState },
		m_vertexLayout{ std::move(other.m_vertexLayout) }
	{
		// GetLayoutDesc gets a new object with the updated pointers.
		m_pipelineState.InputLayout = m_vertexLayout.GetLayoutDesc();
	}
	GraphicsPipelineBuilder& operator=(GraphicsPipelineBuilder&& other) noexcept
	{
		m_pipelineState = other.m_pipelineState;
		m_vertexLayout  = std::move(other.m_vertexLayout);

		// GetLayoutDesc gets a new object with the updated pointers.
		m_pipelineState.InputLayout = m_vertexLayout.GetLayoutDesc();

		return *this;
	}
};

using GraphicsPipelineBuilderVS = GraphicsPipelineBuilder<D3D12_GRAPHICS_PIPELINE_STATE_DESC>;
using GraphicsPipelineBuilderMS = GraphicsPipelineBuilder<D3DX12_MESH_SHADER_PIPELINE_STATE_DESC>;

class ComputePipelineBuilder
{
public:
	ComputePipelineBuilder(ID3D12RootSignature* rootSignature)
		: m_pipelineState{
			.pRootSignature = rootSignature,
			.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE
		}
	{}

	ComputePipelineBuilder& SetComputeStage(const D3D12_SHADER_BYTECODE& computeShader) noexcept
	{
		m_pipelineState.CS = computeShader;

		return *this;
	}

	[[nodiscard]]
	D3D12_COMPUTE_PIPELINE_STATE_DESC Get() const noexcept { return m_pipelineState; }

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC m_pipelineState;

public:
	ComputePipelineBuilder(const ComputePipelineBuilder& other) noexcept
		: m_pipelineState{ other.m_pipelineState }
	{}
	ComputePipelineBuilder& operator=(const ComputePipelineBuilder& other) noexcept
	{
		m_pipelineState = other.m_pipelineState;

		return *this;
	}
	ComputePipelineBuilder(ComputePipelineBuilder&& other) noexcept
		: m_pipelineState{ other.m_pipelineState }
	{}
	ComputePipelineBuilder& operator=(ComputePipelineBuilder&& other) noexcept
	{
		m_pipelineState = other.m_pipelineState;

		return *this;
	}
};

class D3DPipelineObject
{
public:
	D3DPipelineObject() : m_pipelineStateObject{} {}

	void CreateGraphicsPipeline(
		ID3D12Device2* device, const GraphicsPipelineBuilderMS& builder
	);
	void CreateGraphicsPipeline(
		ID3D12Device2* device, const GraphicsPipelineBuilderVS& builder
	);
	void CreateComputePipeline(
		ID3D12Device2* device, const ComputePipelineBuilder& builder
	);

	[[nodiscard]]
	ID3D12PipelineState* Get() const noexcept { return m_pipelineStateObject.Get(); }

private:
	void CreatePipelineState(
		ID3D12Device2* device, SIZE_T streamStructSize, void* streamObject
	);

private:
	ComPtr<ID3D12PipelineState> m_pipelineStateObject;

public:
	D3DPipelineObject(const D3DPipelineObject&) = delete;
	D3DPipelineObject& operator=(const D3DPipelineObject&) = delete;

	D3DPipelineObject(D3DPipelineObject&& other) noexcept
		: m_pipelineStateObject{ std::move(other.m_pipelineStateObject) }
	{}
	D3DPipelineObject& operator=(D3DPipelineObject&& other) noexcept
	{
		m_pipelineStateObject = std::move(other.m_pipelineStateObject);

		return *this;
	}
};
#endif
