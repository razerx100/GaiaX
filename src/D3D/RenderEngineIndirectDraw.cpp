#include <RenderEngineIndirectDraw.hpp>
#include <Gaia.hpp>
#include <D3DResourceBarrier.hpp>
#include <VertexLayout.hpp>
#include <Shader.hpp>
#include <cassert>

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
	m_renderPipeline->ResetCounterBuffer(computeCommandList, frameIndex);

	// Compute Pipeline doesn't need to be changed for different Graphics Pipelines
	BindComputePipeline(computeCommandList);

	Gaia::bufferManager->BindBuffersToCompute(computeCommandList, frameIndex);
	m_renderPipeline->DispatchCompute(computeCommandList, frameIndex, m_computeRSLayout);

	Gaia::computeCmdList->Close();
	Gaia::computeQueue->ExecuteCommandLists(computeCommandList);

	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

	Gaia::computeQueue->SignalCommandQueue(Gaia::computeFence->GetFence(), fenceValue);

	// Graphics Stage
	Gaia::graphicsQueue->WaitOnGPU(Gaia::computeFence->GetFence(), fenceValue);

	Gaia::graphicsCmdList->Reset(frameIndex);

	D3DResourceBarrier<2u>().AddBarrier(
		Gaia::swapChain->GetRTV(frameIndex),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
	).AddBarrier(
		m_renderPipeline->GetArgumentBuffer(frameIndex),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
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
	m_renderPipeline->BindGraphicsPipeline(graphicsCommandList, m_graphicsRS->Get());
	Gaia::textureStorage->BindTextures(graphicsCommandList);
	Gaia::bufferManager->BindBuffersToGraphics(graphicsCommandList, frameIndex);
	Gaia::vertexManager->BindVertices(graphicsCommandList);
	m_renderPipeline->DrawModels(m_commandSignature.Get(), graphicsCommandList, frameIndex);
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

	auto computeRS = CreateComputeRootSignature(device);
	auto graphicsRS = CreateGraphicsRootSignature(device);

	m_computeRSLayout = computeRS->GetElementLayout();
	m_graphicsRSLayout = graphicsRS->GetElementLayout();

	m_computeRS = std::move(computeRS);
	m_graphicsRS = std::move(graphicsRS);

	m_computePSO = CreateComputePipelineObject(device, m_computeRS->Get());

	Gaia::bufferManager->SetComputeRootSignatureLayout(m_computeRSLayout);
	Gaia::bufferManager->SetGraphicsRootSignatureLayout(m_graphicsRSLayout);
	Gaia::textureStorage->SetGraphicsRootSignatureLayout(m_graphicsRSLayout);

	m_renderPipeline->ConfigureGraphicsPipelineObject(
		device, m_shaderPath, L"PixelShader.cso", m_graphicsRS->Get()
	);

	CreateCommandSignature(device);
}

void RenderEngineIndirectDraw::InitiatePipelines(std::uint32_t bufferCount) noexcept {
	m_renderPipeline = std::make_unique<RenderPipelineIndirectDraw>(bufferCount);
}

void RenderEngineIndirectDraw::RecordModelData(
	const std::vector<std::shared_ptr<IModel>>& models
) noexcept {
	m_renderPipeline->RecordIndirectArguments(models);
}

void RenderEngineIndirectDraw::CreateBuffers(ID3D12Device* device) {
	m_renderPipeline->CreateBuffers(device);
}

void RenderEngineIndirectDraw::ReserveBuffers(ID3D12Device* device) {
	m_renderPipeline->ReserveBuffers(device);
}

void RenderEngineIndirectDraw::RecordResourceUploads(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_renderPipeline->RecordResourceUpload(copyList);
}

void RenderEngineIndirectDraw::ReleaseUploadResources() noexcept {
	m_renderPipeline->ReleaseUploadResource();
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

std::unique_ptr<RootSignatureDynamic> RenderEngineIndirectDraw::CreateComputeRootSignature(
	ID3D12Device* device
) const noexcept {
	auto signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::ModelData, false, 0u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::IndirectArgsSRV, false, 1u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::IndirectArgsUAV, false, 0u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::IndirectArgsCounterUAV, false, 1u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_ALL, RootSigElement::Camera, 0u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_ALL, RootSigElement::CullingData, 1u
	).CompileSignature().CreateSignature(device);

	return signature;
}

std::unique_ptr<D3DPipelineObject> RenderEngineIndirectDraw::CreateComputePipelineObject(
	ID3D12Device* device, ID3D12RootSignature* computeRootSignature
) const noexcept {
	auto cs = std::make_unique<Shader>();
	cs->LoadBinary(m_shaderPath + L"ComputeShader.cso");

	auto pso = std::make_unique<D3DPipelineObject>();
	pso->CreateComputePipelineState(device, computeRootSignature, cs->GetByteCode());

	return pso;
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
	desc.ByteStride = static_cast<UINT>(sizeof(IndirectCommand));
	desc.NumArgumentDescs = static_cast<UINT>(std::size(arguments));
	desc.pArgumentDescs = std::data(arguments);

	assert(m_graphicsRS->Get() != nullptr && "Graphics RootSignature not initialised");

	device->CreateCommandSignature(
		&desc, m_graphicsRS->Get(), IID_PPV_ARGS(&m_commandSignature)
	);
}

void RenderEngineIndirectDraw::BindComputePipeline(
	ID3D12GraphicsCommandList* computeCommandList
) const noexcept {
	computeCommandList->SetPipelineState(m_computePSO->Get());
	computeCommandList->SetComputeRootSignature(m_computeRS->Get());
}
