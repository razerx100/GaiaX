#include <RenderPipeline.hpp>
#include <ranges>
#include <algorithm>
#include <D3DThrowMacros.hpp>
#include <array>
#include <Gaia.hpp>

RenderPipeline::RenderPipeline(std::uint32_t frameCount) noexcept
	: m_modelCount(0u), m_frameCount{ frameCount }, m_modelBufferPerFrameSize{ 0u },
	m_modelBuffers{ ResourceType::cpuWrite } {}

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
		m_commandSignature.Get(), m_modelCount, m_commandBuffer->Get(),
		sizeof(IndirectCommand) * m_modelCount * frameIndex, nullptr, 0u
	);
}

void RenderPipeline::ReserveCommandBuffers(ID3D12Device* device) {
	m_modelCount = static_cast<UINT>(std::size(m_opaqueModels));

	auto [gpuBuffer, uploadBuffer] = Gaia::heapManager->AddUploadAbleBuffer(
		m_modelCount * sizeof(IndirectCommand) * m_frameCount
	);

	m_commandBuffer = std::move(gpuBuffer);
	m_commandUploadBuffer = std::move(uploadBuffer);

	auto [cmdBufferCpuDescriptor, cmdBufferGpuDescriptor] =
		Gaia::descriptorTable->ReserveDescriptors(m_frameCount);

	m_commandDescriptorHandle = std::move(cmdBufferCpuDescriptor);

	auto [modelBufferCpuDescriptor, modelBufferGpuDescriptor] =
		Gaia::descriptorTable->ReserveDescriptors(m_frameCount);

	m_modelBuffers.SetDescriptorHandles(
		std::move(modelBufferCpuDescriptor), std::move(modelBufferGpuDescriptor),
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	);
	m_modelBuffers.SetBufferInfo(
		device, static_cast<UINT>(sizeof(ModelConstantBuffer)), m_modelCount, m_frameCount
	);
}

void RenderPipeline::CreateCommandBuffers(ID3D12Device* device) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = m_modelCount;
	srvDesc.Buffer.StructureByteStride = static_cast<UINT>(sizeof(IndirectCommand));
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	UINT descriptorSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ *m_commandDescriptorHandle };

	for (size_t index = 0u; index < m_frameCount; ++index) {
		srvDesc.Buffer.FirstElement = m_modelCount * index;
		device->CreateShaderResourceView(m_commandBuffer->Get(), &srvDesc, cpuHandle);

		cpuHandle.ptr += descriptorSize;
	}

	std::uint8_t* cpuPtr = Gaia::cpuWriteBuffer->GetCPUStartAddress();
	D3D12_GPU_VIRTUAL_ADDRESS gpuPtr = Gaia::cpuWriteBuffer->GetGPUStartAddress();

	m_modelBuffers.CreateDescriptorView(device);

	std::vector<IndirectCommand> commands;
	constexpr size_t modelBufferSize = sizeof(ModelConstantBuffer);

	for (size_t frame = 0u; frame < m_frameCount; ++frame) {
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
	}

	m_commandUploadBuffer->MapBuffer();

	std::uint8_t* commandCPUPtr = m_commandUploadBuffer->GetCPUWPointer();
	memcpy(
		commandCPUPtr, std::data(commands), sizeof(IndirectCommand) * std::size(commands)
	);
}

void RenderPipeline::BindComputePipeline(
	ID3D12GraphicsCommandList* computeCommandList
) const noexcept {}

void RenderPipeline::DispatchCompute(
	ID3D12GraphicsCommandList* computeCommandList
) const noexcept {}
