#include <RenderPipeline.hpp>
#include <ranges>
#include <algorithm>
#include <D3DThrowMacros.hpp>
#include <array>
#include <Gaia.hpp>

RenderPipeline::RenderPipeline(std::uint32_t frameCount) noexcept
	: m_modelCount(0u), m_frameCount{ frameCount }, m_modelBufferPerFrameSize{ 0u },
	m_commandDescriptorOffset{ 0u }, m_modelBuffers{ ResourceType::cpuWrite } {}

void RenderPipeline::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

void RenderPipeline::AddComputePipelineObject() noexcept {}

void RenderPipeline::AddComputeRootSignature(
	std::unique_ptr<RootSignatureBase> signature
) noexcept {
	m_computeRS = std::move(signature);
}

void RenderPipeline::AddGraphicsPipelineObject(
	std::unique_ptr<PipelineObjectGFX> pso
) noexcept {
	m_graphicPSO = std::move(pso);
}

void RenderPipeline::AddGraphicsRootSignature(
	std::unique_ptr<RootSignatureBase> signature
) noexcept {
	m_graphicsRS = std::move(signature);
}

void RenderPipeline::UpdateModels(size_t frameIndex) const noexcept {
	size_t offset = 0u;
	constexpr size_t bufferStride = sizeof(ModelConstantBuffer);

	for (auto& model : m_opaqueModels) {
		ModelConstantBuffer modelBuffer{};
		modelBuffer.textureIndex = model->GetTextureIndex();
		modelBuffer.uvInfo = model->GetUVInfo();
		modelBuffer.modelMatrix = model->GetModelMatrix();

		memcpy(
			m_modelBuffers.GetCPUWPointer(frameIndex) + offset,
			&modelBuffer, bufferStride
		);

		offset += bufferStride;
	}
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

	HRESULT hr{};
	D3D_THROW_FAILED(hr,
		device->CreateCommandSignature(
			&desc, m_graphicsRS->Get(), __uuidof(ID3D12CommandSignature), &m_commandSignature
		)
	);
}

void RenderPipeline::DrawModels(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) const noexcept {
	graphicsCommandList->SetGraphicsRootDescriptorTable(
		1u, m_modelBuffers.GetGPUDescriptorHandle(frameIndex)
	);

	graphicsCommandList->ExecuteIndirect(
		m_commandSignature.Get(), m_modelCount, m_commandBuffer.GetResource(),
		sizeof(IndirectCommand) * m_modelCount * frameIndex, nullptr, 0u
	);
}

void RenderPipeline::ReserveCommandBuffers(ID3D12Device* device) {
	m_modelCount = static_cast<UINT>(std::size(m_opaqueModels));

	const size_t commandDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);

	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_commandBuffer.SetDescriptorOffset(commandDescriptorOffset, descriptorSize);
	m_modelBuffers.SetBufferInfo(
		device, static_cast<UINT>(sizeof(IndirectCommand)), m_modelCount, m_frameCount
	);

	size_t modelBufferDescriptorOffset = Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(
		m_frameCount
	);

	m_modelBuffers.SetDescriptorOffset(modelBufferDescriptorOffset, descriptorSize);
	m_modelBuffers.SetBufferInfo(
		device, static_cast<UINT>(sizeof(ModelConstantBuffer)), m_modelCount, m_frameCount
	);
}

void RenderPipeline::CreateCommandBuffers(ID3D12Device* device) {
	const D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart =
		Gaia::descriptorTable->GetUploadDescriptorStart();

	const D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart =
		Gaia::descriptorTable->GetGPUDescriptorStart();

	m_modelBuffers.CreateDescriptorView(
		device, uploadDescriptorStart, gpuDescriptorStart,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);
	m_commandBuffer.CreateDescriptorView(
		device, uploadDescriptorStart, gpuDescriptorStart,
		D3D12_RESOURCE_STATE_COPY_DEST
	);

	std::vector<IndirectCommand> commands;
	constexpr size_t modelBufferSize = sizeof(ModelConstantBuffer);

	for (size_t index = 0u; index < std::size(m_opaqueModels); ++index) {
		IndirectCommand command{};
		command.modelIndex = static_cast<std::uint32_t>(index);

		const auto& model = m_opaqueModels[index];

		command.drawIndexed.BaseVertexLocation = 0u;
		command.drawIndexed.IndexCountPerInstance = model->GetIndexCount();
		command.drawIndexed.StartIndexLocation = model->GetIndexOffset();
		command.drawIndexed.InstanceCount = 1u;
		command.drawIndexed.StartInstanceLocation = 0u;

		commands.emplace_back(command);
	}

	for (size_t frame = 0u; frame < m_frameCount; ++frame) {
		std::uint8_t* commandCPUPtr = m_commandBuffer.GetCPUWPointer(frame);
		memcpy(
			commandCPUPtr, std::data(commands), sizeof(IndirectCommand) * std::size(commands)
		);
	}
}

void RenderPipeline::BindComputePipeline(
	ID3D12GraphicsCommandList* computeCommandList
) const noexcept {}

void RenderPipeline::DispatchCompute(
	ID3D12GraphicsCommandList* computeCommandList
) const noexcept {}

void RenderPipeline::RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept {
	m_commandBuffer.RecordResourceUpload(copyList);
}

void RenderPipeline::ReleaseUploadResource() noexcept {
	m_commandBuffer.ReleaseUploadResource();
}
