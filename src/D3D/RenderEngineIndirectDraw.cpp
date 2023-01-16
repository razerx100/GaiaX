#include <RenderEngineIndirectDraw.hpp>
#include <Gaia.hpp>
#include <D3DResourceBarrier.hpp>
#include <VertexLayout.hpp>
#include <Shader.hpp>
#include <cassert>

RenderEngineIndirectDraw::RenderEngineIndirectDraw(const Args& arguments)
	: m_computePipeline{ arguments.frameCount.value() } {}

void RenderEngineIndirectDraw::ExecutePreRenderStage(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	// Compute Stage
	ID3D12GraphicsCommandList* computeCommandList = Gaia::computeCmdList->GetCommandList();
	Gaia::computeCmdList->Reset(frameIndex);
	// Need some barrier stuff

	ID3D12DescriptorHeap* ppHeap[] = { Gaia::descriptorTable->GetDescHeapRef() };
	computeCommandList->SetDescriptorHeaps(1u, ppHeap);

	// Record compute commands
	m_computePipeline.ResetCounterBuffer(computeCommandList, frameIndex);

	// Compute Pipeline doesn't need to be changed for different Graphics Pipelines
	m_computePipeline.BindComputePipeline(computeCommandList);

	Gaia::bufferManager->BindBuffersToCompute(computeCommandList, frameIndex);
	m_computePipeline.DispatchCompute(computeCommandList, frameIndex);

	Gaia::computeCmdList->Close();
	Gaia::computeQueue->ExecuteCommandLists(computeCommandList);

	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

	Gaia::computeQueue->SignalCommandQueue(Gaia::computeFence->GetFence(), fenceValue);

	// Graphics Stage
	Gaia::graphicsQueue->WaitOnGPU(Gaia::computeFence->GetFence(), fenceValue);

	Gaia::graphicsCmdList->Reset(frameIndex);

	D3DResourceBarrier<1u>().AddBarrier(
		Gaia::swapChain->GetRTV(frameIndex),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
	).RecordBarriers(graphicsCommandList);

	graphicsCommandList->SetDescriptorHeaps(1u, ppHeap);

	graphicsCommandList->RSSetViewports(1u, Gaia::viewportAndScissor->GetViewportRef());
	graphicsCommandList->RSSetScissorRects(1u, Gaia::viewportAndScissor->GetScissorRef());

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = Gaia::swapChain->GetRTVHandle(frameIndex);

	Gaia::swapChain->ClearRTV(
		graphicsCommandList, std::data(m_backgroundColour), rtvHandle
	);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = Gaia::Resources::depthBuffer->GetDSVHandle();

	Gaia::Resources::depthBuffer->ClearDSV(graphicsCommandList, dsvHandle);

	graphicsCommandList->OMSetRenderTargets(1u, &rtvHandle, FALSE, &dsvHandle);
}

void RenderEngineIndirectDraw::RecordDrawCommands(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	// One Pipeline needs to be bound before Descriptors can be bound.
	ID3D12RootSignature* graphicsRS = m_graphicsRS->Get();

	m_graphicsPipeline0->BindGraphicsPipeline(graphicsCommandList, graphicsRS);

	Gaia::textureStorage->BindTextures(graphicsCommandList);
	Gaia::bufferManager->BindBuffersToGraphics(graphicsCommandList, frameIndex);
	m_vertexManager.BindVertexAndIndexBuffer(graphicsCommandList);

	m_graphicsPipeline0->DrawModels(
		m_commandSignature.Get(), graphicsCommandList,
		m_computePipeline.GetArgumentBuffer(frameIndex),
		m_computePipeline.GetCounterBuffer(frameIndex)
	);

	for (auto& graphicsPipeline : m_graphicsPipelines) {
		graphicsPipeline->BindGraphicsPipeline(graphicsCommandList, graphicsRS);
		graphicsPipeline->DrawModels(
			m_commandSignature.Get(), graphicsCommandList,
			m_computePipeline.GetArgumentBuffer(frameIndex),
			m_computePipeline.GetCounterBuffer(frameIndex)
		);
	}
}

void RenderEngineIndirectDraw::Present(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	D3DResourceBarrier().AddBarrier(
		Gaia::swapChain->GetRTV(frameIndex),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	).RecordBarriers(graphicsCommandList);

	Gaia::graphicsCmdList->Close();
	Gaia::graphicsQueue->ExecuteCommandLists(graphicsCommandList);

	Gaia::swapChain->PresentWithTear();
}

void RenderEngineIndirectDraw::ExecutePostRenderStage() {
	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

	Gaia::graphicsQueue->SignalCommandQueue(Gaia::graphicsFence->GetFence(), fenceValue);
	Gaia::graphicsFence->AdvanceValueInQueue();
	Gaia::graphicsFence->WaitOnCPUConditional();
	Gaia::graphicsFence->IncreaseFrontValue(fenceValue);
}

void RenderEngineIndirectDraw::ConstructPipelines() {
	ID3D12Device* device = Gaia::device->GetDeviceRef();

	m_computePipeline.CreateComputeRootSignature(device);
	auto graphicsRS = CreateGraphicsRootSignature(device);

	m_graphicsRSLayout = graphicsRS->GetElementLayout();

	m_graphicsRS = std::move(graphicsRS);

	ID3D12RootSignature* graphicsRootSig = m_graphicsRS->Get();

	m_graphicsPipeline0->CreateGraphicsPipeline(device,  graphicsRootSig, m_shaderPath);
	for (auto& graphicsPipeline : m_graphicsPipelines)
		graphicsPipeline->CreateGraphicsPipeline(device, graphicsRootSig, m_shaderPath);

	m_computePipeline.CreateComputePipelineObject(device, m_shaderPath);
	Gaia::bufferManager->SetComputeRootSignatureLayout(
		m_computePipeline.GetComputeRSLayout()
	);
	Gaia::bufferManager->SetGraphicsRootSignatureLayout(m_graphicsRSLayout);
	Gaia::textureStorage->SetGraphicsRootSignatureLayout(m_graphicsRSLayout);

	CreateCommandSignature(device);
}

void RenderEngineIndirectDraw::RecordModelDataSet(
	const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& pixelShader
) noexcept {
	auto graphicsPipeline = std::make_unique<GraphicsPipelineIndirectDraw>();
	graphicsPipeline->ConfigureGraphicsPipelineObject(
		pixelShader, static_cast<std::uint32_t>(std::size(models)),
		m_computePipeline.GetCurrentModelCount(), m_computePipeline.GetCounterCount()
	);

	// old currentModelCount hold the modelCountOffset value
	m_computePipeline.RecordIndirectArguments(models);

	if (!m_graphicsPipeline0)
		m_graphicsPipeline0 = std::move(graphicsPipeline);
	else
		m_graphicsPipelines.emplace_back(std::move(graphicsPipeline));
}

void RenderEngineIndirectDraw::CreateBuffers(ID3D12Device* device) {
	m_computePipeline.CreateBuffers(device);
	m_vertexManager.CreateBuffers(device);
}

void RenderEngineIndirectDraw::ReserveBuffers(ID3D12Device* device) {
	m_computePipeline.ReserveBuffers(device);
	m_vertexManager.ReserveBuffers(device);
}

void RenderEngineIndirectDraw::RecordResourceUploads(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_computePipeline.RecordResourceUpload(copyList);
	m_vertexManager.RecordResourceUploads(copyList);
}

void RenderEngineIndirectDraw::ReleaseUploadResources() noexcept {
	m_computePipeline.ReleaseUploadResource();
	m_vertexManager.ReleaseUploadResources();
}

std::unique_ptr<RootSignatureDynamic> RenderEngineIndirectDraw::CreateGraphicsRootSignature(
	ID3D12Device* device
) const noexcept {
	auto signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, D3D12_SHADER_VISIBILITY_PIXEL,
		RootSigElement::Textures, true, 1u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_VERTEX,
		RootSigElement::ModelData, false, 0u
	).AddConstants(
		1u, D3D12_SHADER_VISIBILITY_VERTEX, RootSigElement::ModelIndex, 0u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_VERTEX, RootSigElement::Camera, 1u
	).CompileSignature().CreateSignature(device);

	return signature;
}

void RenderEngineIndirectDraw::CreateCommandSignature(ID3D12Device* device) {
	static constexpr size_t modelIndex = static_cast<size_t>(RootSigElement::ModelIndex);

	std::array<D3D12_INDIRECT_ARGUMENT_DESC, 2u> arguments{};
	arguments[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
	arguments[0].Constant.RootParameterIndex = m_graphicsRSLayout[modelIndex];
	arguments[0].Constant.DestOffsetIn32BitValues = 0u;
	arguments[0].Constant.Num32BitValuesToSet = 1u;
	arguments[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

	D3D12_COMMAND_SIGNATURE_DESC desc{};
	desc.ByteStride = static_cast<UINT>(sizeof(IndirectArguments));
	desc.NumArgumentDescs = static_cast<UINT>(std::size(arguments));
	desc.pArgumentDescs = std::data(arguments);

	assert(m_graphicsRS->Get() != nullptr && "Graphics RootSignature not initialised");

	device->CreateCommandSignature(
		&desc, m_graphicsRS->Get(), IID_PPV_ARGS(&m_commandSignature)
	);
}

void RenderEngineIndirectDraw::AddGlobalVertices(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) noexcept {
	m_vertexManager.AddGlobalVertices(
		std::move(vertices), vertexBufferSize, std::move(indices), indexBufferSize
	);
}

void RenderEngineIndirectDraw::CopyData(std::atomic_size_t& workCount) const noexcept {
	m_vertexManager.CopyData(workCount);
}
