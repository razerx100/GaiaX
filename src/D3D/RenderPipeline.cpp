#include <RenderPipeline.hpp>
#include <ranges>
#include <algorithm>
#include <array>
#include <Gaia.hpp>

RenderPipeline::RenderPipeline(std::uint32_t frameCount) noexcept
	: m_modelCount(0u), m_frameCount{ frameCount }, m_modelBufferPerFrameSize{ 0u },
	m_commandDescriptorOffset{ 0u } {}

void RenderPipeline::AddComputePipelineObject(
	std::unique_ptr<D3DPipelineObject> pso
) noexcept {
	m_computePSO = std::move(pso);
}

void RenderPipeline::AddComputeRootSignature(
	std::unique_ptr<RootSignatureBase> signature
) noexcept {
	m_computeRS = std::move(signature);
}

void RenderPipeline::AddGraphicsPipelineObject(
	std::unique_ptr<D3DPipelineObject> pso
) noexcept {
	m_graphicPSO = std::move(pso);
}

void RenderPipeline::AddGraphicsRootSignature(
	std::unique_ptr<RootSignatureBase> signature
) noexcept {
	m_graphicsRS = std::move(signature);
}

void RenderPipeline::BindGraphicsPipeline(
	ID3D12GraphicsCommandList* graphicsCommandList
) const noexcept {
	graphicsCommandList->SetPipelineState(m_graphicPSO->Get());
	graphicsCommandList->SetGraphicsRootSignature(m_graphicsRS->Get());
	graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void RenderPipeline::CreateCommandSignature(ID3D12Device* device) {
	std::array<D3D12_INDIRECT_ARGUMENT_DESC, 2u> arguments{};
	arguments[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
	arguments[0].Constant.RootParameterIndex = 2u;
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

void RenderPipeline::DrawModels(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) const noexcept {
	graphicsCommandList->ExecuteIndirect(
		m_commandSignature.Get(), m_modelCount, m_commandBuffer.GetResource(),
		sizeof(IndirectCommand) * m_modelCount * frameIndex, nullptr, 0u
	);
}

void RenderPipeline::ReserveBuffers(ID3D12Device* device) {
	const size_t commandDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);

	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_commandBuffer.SetDescriptorOffset(commandDescriptorOffset, descriptorSize);
	m_commandBuffer.SetBufferInfo(
		device, static_cast<UINT>(sizeof(IndirectCommand)), m_modelCount, m_frameCount
	);
}

void RenderPipeline::RecordIndirectArguments(
	const std::vector<std::shared_ptr<IModel>>& models
) noexcept {
	for (size_t index = 0u; index < std::size(models); ++index) {
		IndirectCommand command{};
		command.modelIndex = static_cast<std::uint32_t>(std::size(m_indirectCommands));

		const auto& model = models[index];

		command.drawIndexed.BaseVertexLocation = 0u;
		command.drawIndexed.IndexCountPerInstance = model->GetIndexCount();
		command.drawIndexed.StartIndexLocation = model->GetIndexOffset();
		command.drawIndexed.InstanceCount = 1u;
		command.drawIndexed.StartInstanceLocation = 0u;

		m_indirectCommands.emplace_back(command);
	}

	m_modelCount += static_cast<UINT>(std::size(models));
}

void RenderPipeline::CreateBuffers(ID3D12Device* device) {
	const D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart =
		Gaia::descriptorTable->GetUploadDescriptorStart();

	const D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart =
		Gaia::descriptorTable->GetGPUDescriptorStart();

	m_commandBuffer.CreateDescriptorView(
		device, uploadDescriptorStart, gpuDescriptorStart, D3D12_RESOURCE_STATE_COPY_DEST
	);

	for (size_t frame = 0u; frame < m_frameCount; ++frame) {
		std::uint8_t* commandCPUPtr = m_commandBuffer.GetCPUWPointer(frame);
		memcpy(
			commandCPUPtr, std::data(m_indirectCommands),
			sizeof(IndirectCommand) * std::size(m_indirectCommands)
		);
	}

	m_indirectCommands = std::vector<IndirectCommand>();
}

void RenderPipeline::BindComputePipeline(
	ID3D12GraphicsCommandList* computeCommandList
) const noexcept {
	computeCommandList->SetPipelineState(m_computePSO->Get());
	computeCommandList->SetComputeRootSignature(m_computeRS->Get());
}

void RenderPipeline::DispatchCompute(
	ID3D12GraphicsCommandList* computeCommandList, size_t frameIndex
) const noexcept {
	computeCommandList->SetComputeRootDescriptorTable(
		1u, m_commandBuffer.GetGPUDescriptorHandle(frameIndex)
	);

	computeCommandList->Dispatch(
		static_cast<UINT>(std::ceil(m_modelCount / THREADBLOCKSIZE)), 1u, 1u
	);
}

void RenderPipeline::RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept {
	m_commandBuffer.RecordResourceUpload(copyList);
}

void RenderPipeline::ReleaseUploadResource() noexcept {
	m_commandBuffer.ReleaseUploadResource();
}

D3D12_RESOURCE_BARRIER RenderPipeline::GetTransitionBarrier(
	D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState
) const noexcept {
	return ::GetTransitionBarrier(m_commandBuffer.GetResource(), beforeState, afterState);
}
