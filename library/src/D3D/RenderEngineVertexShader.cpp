#include <RenderEngineVertexShader.hpp>
#include <Gaia.hpp>
#include <D3DResourceBarrier.hpp>
#include <VertexLayout.hpp>
#include <cassert>

/*
// Vertex Shader
RenderEngineVertexShader::RenderEngineVertexShader(ID3D12Device* device)
	: RenderEngineBase{ device } {}

void RenderEngineVertexShader::AddGVerticesAndIndices(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
) noexcept {
	//m_vertexManager.AddGVerticesAndIndices(std::move(gVertices), std::move(gIndices));
}

void RenderEngineVertexShader::CreateBuffers(ID3D12Device* device) {
//	m_vertexManager.CreateBuffers(device);
	_createBuffers(device);
}

void RenderEngineVertexShader::ReserveBuffersDerived(ID3D12Device* device) {
//	m_vertexManager.ReserveBuffers(device);
	_reserveBuffers(device);
}

void RenderEngineVertexShader::RecordResourceUploads(
	ID3D12GraphicsCommandList* copyList
) noexcept {
//	m_vertexManager.RecordResourceUploads(copyList);
	_recordResourceUploads(copyList);
}

void RenderEngineVertexShader::ReleaseUploadResources() noexcept {
//	m_vertexManager.ReleaseUploadResources();
	_releaseUploadResources();
}

void RenderEngineVertexShader::_createBuffers([[maybe_unused]] ID3D12Device* device) {}
void RenderEngineVertexShader::_reserveBuffers([[maybe_unused]] ID3D12Device* device) {}
void RenderEngineVertexShader::_recordResourceUploads(
	[[maybe_unused]] ID3D12GraphicsCommandList* copyList
) noexcept {}
void RenderEngineVertexShader::_releaseUploadResources() noexcept {}

std::unique_ptr<RootSignatureDynamic> RenderEngineVertexShader::CreateGraphicsRootSignature(
	ID3D12Device* device
) const noexcept {
	auto signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, D3D12_SHADER_VISIBILITY_PIXEL,
		RootSigElement::Textures, true, 0u, 1u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_VERTEX,
		RootSigElement::ModelData, false, 0u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_PIXEL,
		RootSigElement::MaterialData, false, 1u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_PIXEL,
		RootSigElement::LightData, false, 2u
	).AddConstants(
		1u, D3D12_SHADER_VISIBILITY_VERTEX, RootSigElement::ModelInfo, 0u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_VERTEX, RootSigElement::Camera, 1u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_PIXEL, RootSigElement::PixelData, 2u
	).CompileSignature().CreateSignature(device);

	return signature;
}

void RenderEngineVertexShader::BindGraphicsBuffers(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	BindCommonGraphicsBuffers(graphicsCommandList, frameIndex);
//	m_vertexManager.BindVertexAndIndexBuffer(graphicsCommandList);
}

void RenderEngineVertexShader::ExecuteRenderStage(size_t frameIndex) {
	ID3D12GraphicsCommandList* graphicsCommandList = Gaia::graphicsCmdList->Get();

	ExecutePreRenderStage(graphicsCommandList, frameIndex);
	RecordDrawCommands(graphicsCommandList, frameIndex);
}

// Indirect Draw
RenderEngineIndirectDraw::RenderEngineIndirectDraw(ID3D12Device* device, std::uint32_t frameCount)
	: RenderEngineVertexShader{ device } {}//, m_computePipeline{ frameCount } {}

void RenderEngineIndirectDraw::ExecuteComputeStage(size_t frameIndex) {
	ID3D12GraphicsCommandList* computeCommandList = Gaia::computeCmdList->Get();
	Gaia::computeCmdList->Reset();

	//ID3D12DescriptorHeap* descriptorHeap[] = { Gaia::descriptorTable->GetDescHeapRef() };
	//computeCommandList->SetDescriptorHeaps(1u, descriptorHeap);

	// Record compute commands
	//m_computePipeline.ResetCounterBuffer(computeCommandList, frameIndex);

//	m_computePipeline.BindComputePipeline(computeCommandList);

	//Gaia::bufferManager->BindBuffersToCompute(computeCommandList, frameIndex);
//	m_computePipeline.DispatchCompute(computeCommandList, frameIndex);

	Gaia::computeCmdList->Close();
	Gaia::computeQueue->ExecuteCommandLists(computeCommandList);

	UINT64 fenceValue = Gaia::graphicsFence->GetFrontValue();

	Gaia::computeQueue->SignalCommandQueue(Gaia::computeFence->GetFence(), fenceValue);

	Gaia::graphicsQueue->WaitOnGPU(Gaia::computeFence->GetFence(), fenceValue);
}

void RenderEngineIndirectDraw::ExecutePreRenderStage(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	ExecuteComputeStage(frameIndex);
	ExecutePreGraphicsStage(graphicsCommandList, frameIndex);
}

void RenderEngineIndirectDraw::UpdateModelBuffers(size_t frameIndex) const noexcept {
//	Gaia::bufferManager->Update<false>(frameIndex);
}

void RenderEngineIndirectDraw::RecordDrawCommands(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	ID3D12RootSignature* graphicsRS = m_graphicsRS->Get();

	// One Pipeline needs to be bound before Descriptors can be bound.
	m_graphicsPipeline0->BindGraphicsPipeline(graphicsCommandList, graphicsRS);
	BindGraphicsBuffers(graphicsCommandList, frameIndex);

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

void RenderEngineIndirectDraw::ConstructPipelines() {
	ID3D12Device2* device = Gaia::device->GetDevice();

	ConstructGraphicsRootSignature(device);
	CreateGraphicsPipelines(device, m_graphicsPipeline0, m_graphicsPipelines);

	m_computePipeline.CreateComputeRootSignature(device);

	m_computePipeline.CreateComputePipelineObject(device, m_shaderPath);
	Gaia::bufferManager->SetComputeRootSignatureLayout(
		m_computePipeline.GetComputeRSLayout()
	);

	CreateCommandSignature(device);
}

void RenderEngineIndirectDraw::RecordModelDataSet(
	const std::vector<std::shared_ptr<Model>>& models, const std::wstring& pixelShader
) noexcept {
	auto graphicsPipeline = std::make_unique<GraphicsPipelineIndirectDraw>();

	// old currentModelCount hold the modelCountOffset value
//	graphicsPipeline->ConfigureGraphicsPipelineObject(
//		pixelShader, static_cast<std::uint32_t>(std::size(models)),
//		m_computePipeline.GetCurrentModelCount(), m_computePipeline.GetCounterCount()
//	);

//	m_computePipeline.RecordIndirectArguments(models);

	if (!m_graphicsPipeline0)
		m_graphicsPipeline0 = std::move(graphicsPipeline);
	else
		m_graphicsPipelines.emplace_back(std::move(graphicsPipeline));
}

void RenderEngineIndirectDraw::_createBuffers(ID3D12Device* device) {
//	m_computePipeline.CreateBuffers(device);
}

void RenderEngineIndirectDraw::_reserveBuffers(ID3D12Device* device) {
//	m_computePipeline.ReserveBuffers(device);
}

void RenderEngineIndirectDraw::_recordResourceUploads(
	ID3D12GraphicsCommandList* copyList
) noexcept {
//	m_computePipeline.RecordResourceUpload(copyList);
}

void RenderEngineIndirectDraw::_releaseUploadResources() noexcept {
//	m_computePipeline.ReleaseUploadResource();
}

void RenderEngineIndirectDraw::CreateCommandSignature(ID3D12Device* device) {
	static constexpr size_t modelInfoIndex = static_cast<size_t>(RootSigElement::ModelInfo);

	std::array<D3D12_INDIRECT_ARGUMENT_DESC, 2u> arguments{};
	arguments[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
	arguments[0].Constant.RootParameterIndex = m_graphicsRSLayout[modelInfoIndex];
	arguments[0].Constant.DestOffsetIn32BitValues = 0u;
	arguments[0].Constant.Num32BitValuesToSet = 1u;
	arguments[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

	D3D12_COMMAND_SIGNATURE_DESC desc{};
	desc.ByteStride = static_cast<UINT>(sizeof(ModelDrawArguments));
	desc.NumArgumentDescs = static_cast<UINT>(std::size(arguments));
	desc.pArgumentDescs = std::data(arguments);

	assert(m_graphicsRS->Get() != nullptr && "Graphics RootSignature not initialised");

	device->CreateCommandSignature(
		&desc, m_graphicsRS->Get(), IID_PPV_ARGS(&m_commandSignature)
	);
}

// Individual Draw
RenderEngineIndividualDraw::RenderEngineIndividualDraw(ID3D12Device* device)
	: RenderEngineVertexShader{ device } {}

void RenderEngineIndividualDraw::ExecutePreRenderStage(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	ExecutePreGraphicsStage(graphicsCommandList, frameIndex);
}

void RenderEngineIndividualDraw::UpdateModelBuffers(size_t frameIndex) const noexcept {
//	Gaia::bufferManager->Update<true>(frameIndex);
}

void RenderEngineIndividualDraw::RecordDrawCommands(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	ID3D12RootSignature* graphicsRS = m_graphicsRS->Get();

	// One Pipeline needs to be bound before Descriptors can be bound.
	m_graphicsPipeline0->BindGraphicsPipeline(graphicsCommandList, graphicsRS);
	BindGraphicsBuffers(graphicsCommandList, frameIndex);

	m_graphicsPipeline0->DrawModels(graphicsCommandList, m_modelArguments, m_graphicsRSLayout);

	for (auto& graphicsPipeline : m_graphicsPipelines) {
		graphicsPipeline->BindGraphicsPipeline(graphicsCommandList, graphicsRS);
		graphicsPipeline->DrawModels(graphicsCommandList, m_modelArguments, m_graphicsRSLayout);
	}
}

void RenderEngineIndividualDraw::ConstructPipelines() {
	ID3D12Device2* device = Gaia::device->GetDevice();

	ConstructGraphicsRootSignature(device);
	//CreateGraphicsPipelines(device, m_graphicsPipeline0, m_graphicsPipelines);
}

void RenderEngineIndividualDraw::RecordModelDataSet(
	const std::vector<std::shared_ptr<Model>>& models, const std::wstring& pixelShader
) noexcept {
	auto graphicsPipeline = std::make_unique<GraphicsPipelineIndividualDraw>();

	//graphicsPipeline->ConfigureGraphicsPipelineObject(
	//	pixelShader, static_cast<std::uint32_t>(std::size(models)), std::size(m_modelArguments)
	//);

	RecordModelArguments(models);

	if (!m_graphicsPipeline0)
		m_graphicsPipeline0 = std::move(graphicsPipeline);
	else
		m_graphicsPipelines.emplace_back(std::move(graphicsPipeline));
}

void RenderEngineIndividualDraw::RecordModelArguments(
	const std::vector<std::shared_ptr<Model>>& models
) noexcept {
	for (const auto& model : models) {
		D3D12_DRAW_INDEXED_ARGUMENTS arguments{
			//.IndexCountPerInstance = model->GetIndexCount(),
			.InstanceCount = 1u,
			//.StartIndexLocation = model->GetIndexOffset(),
			.BaseVertexLocation = 0u,
			.StartInstanceLocation = 0u
		};

		//ModelDrawArguments modelArgs{
	//		.modelIndex = static_cast<std::uint32_t>(std::size(m_modelArguments)),
		//	.drawIndexed = arguments
		//};

		//m_modelArguments.emplace_back(modelArgs);
	}
}
*/
