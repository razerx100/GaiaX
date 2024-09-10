#include <D3DPipelineObject.hpp>

// D3D Pipeline Object
void D3DPipelineObject::CreateGraphicsPipeline(
	ID3D12Device2* device, const GraphicsPipelineBuilderMS& builder
) {
	CD3DX12_PIPELINE_MESH_STATE_STREAM streamDesc = builder.Get();

	CreatePipelineState(device, sizeof(CD3DX12_PIPELINE_MESH_STATE_STREAM), &streamDesc);
}

void D3DPipelineObject::CreateGraphicsPipeline(
	ID3D12Device2* device, const GraphicsPipelineBuilderVS& builder
) {
	CD3DX12_PIPELINE_STATE_STREAM1 streamDesc = builder.Get();

	CreatePipelineState(device, sizeof(CD3DX12_PIPELINE_STATE_STREAM1), &streamDesc);
}

void D3DPipelineObject::CreateComputePipeline(
	ID3D12Device2* device, const ComputePipelineBuilder& builder
) {
	CD3DX12_PIPELINE_STATE_STREAM1 streamDesc = builder.Get();

	CreatePipelineState(device, sizeof(CD3DX12_PIPELINE_STATE_STREAM1), &streamDesc);
}

void D3DPipelineObject::CreatePipelineState(
	ID3D12Device2* device, SIZE_T streamStructSize, void* streamObject
) {
	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc
	{
		.SizeInBytes                   = streamStructSize,
		.pPipelineStateSubobjectStream = streamObject
	};

	device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pipelineStateObject));
}
