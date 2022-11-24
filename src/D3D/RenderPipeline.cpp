#include <RenderPipeline.hpp>
#include <ranges>
#include <algorithm>
#include <array>
#include <Gaia.hpp>

RenderPipeline::RenderPipeline(std::uint32_t frameCount) noexcept
	: m_modelCount(0u), m_frameCount{ frameCount }, m_modelBufferPerFrameSize{ 0u },
	m_commandBufferUAVs{
		frameCount,
		{ResourceType::gpuOnly, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS}
	},
	m_uavCounterBuffer{ ResourceType::cpuWrite }, m_commandDescriptorOffset{ 0u } {}

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
		m_commandSignature.Get(), m_modelCount, m_commandBufferUAVs[frameIndex].GetResource(),
		m_commandBufferUAVs[frameIndex].GetBufferOffset(0u),
		m_commandBufferUAVs[frameIndex].GetResource(),
		m_commandBufferUAVs[frameIndex].GetCounterOffset(0u)
	);
}

void RenderPipeline::ReserveBuffers(ID3D12Device* device) {
	const size_t commandDescriptorOffsetSRV =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);
	size_t commandDescriptorOffsetUAV =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);

	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	static constexpr auto indirectStructSize = static_cast<UINT>(sizeof(IndirectCommand));

	m_commandBufferSRV.SetDescriptorOffset(commandDescriptorOffsetSRV, descriptorSize);
	m_commandBufferSRV.SetBufferInfo(device, indirectStructSize, m_modelCount, m_frameCount);

	for (auto& commandBufferUAV : m_commandBufferUAVs) {
		commandBufferUAV.SetDescriptorOffset(commandDescriptorOffsetUAV, descriptorSize);
		commandBufferUAV.SetBufferInfo(device, indirectStructSize, m_modelCount, 1u);

		++commandDescriptorOffsetUAV;
	}

	m_uavCounterBuffer.SetBufferInfo(sizeof(UINT));
	m_uavCounterBuffer.ReserveHeapSpace(device);

	m_cullingDataBuffer.SetBufferInfo(sizeof(CullingData));
	m_cullingDataBuffer.ReserveHeapSpace(device);
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

	m_commandBufferSRV.CreateDescriptorView(
		device, uploadDescriptorStart, gpuDescriptorStart, D3D12_RESOURCE_STATE_COPY_DEST
	);

	for (auto& commandBufferUAV : m_commandBufferUAVs)
		commandBufferUAV.CreateDescriptorView(
			device, uploadDescriptorStart, gpuDescriptorStart, D3D12_RESOURCE_STATE_COPY_DEST
		);

	m_uavCounterBuffer.CreateResource(device, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_cullingDataBuffer.CreateResource(device);

	// copy the culling data to the buffer.
	std::uint8_t* cullingBufferPtr = m_cullingDataBuffer.GetCPUWPointer();

	CullingData cullingData{};
	cullingData.commandCount = static_cast<std::uint32_t>(std::size(m_indirectCommands));
	cullingData.xBounds = XBOUNDS;
	cullingData.yBounds = YBOUNDS;
	cullingData.zBounds = ZBOUNDS;

	memcpy(cullingBufferPtr, &cullingData, sizeof(CullingData));

	// copy zero to counter buffer
	std::uint8_t* counterCPUPtr = m_uavCounterBuffer.GetCPUWPointer();
	const UINT zeroValue = 0u;
	memcpy(counterCPUPtr, &zeroValue, sizeof(UINT));

	// Copy Indirect Commands
	for (size_t frame = 0u; frame < m_frameCount; ++frame) {
		std::uint8_t* commandCPUPtr = m_commandBufferSRV.GetBufferCPUWPointer(frame);
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
		1u, m_commandBufferSRV.GetGPUDescriptorHandle(frameIndex)
	);
	computeCommandList->SetComputeRootDescriptorTable(
		2u, m_commandBufferUAVs[frameIndex].GetGPUDescriptorHandle(0u)
	);
	computeCommandList->SetComputeRootConstantBufferView(
		4u, m_cullingDataBuffer.GetGPUAddress()
	);

	computeCommandList->Dispatch(
		static_cast<UINT>(std::ceil(m_modelCount / THREADBLOCKSIZE)), 1u, 1u
	);
}

void RenderPipeline::RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept {
	m_commandBufferSRV.RecordResourceUpload(copyList);
	m_cullingDataBuffer.RecordResourceUpload(copyList);
}

void RenderPipeline::ReleaseUploadResource() noexcept {
	m_commandBufferSRV.ReleaseUploadResource();
	m_cullingDataBuffer.ReleaseUploadResource();
}

D3D12_RESOURCE_BARRIER RenderPipeline::GetTransitionBarrier(
	D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState, size_t frameIndex
) const noexcept {
	return ::GetTransitionBarrier(
		m_commandBufferUAVs[frameIndex].GetResource(), beforeState, afterState
	);
}

void RenderPipeline::ResetCounterBuffer(
	ID3D12GraphicsCommandList* commandList, size_t frameIndex
) const noexcept {
	commandList->CopyBufferRegion(
		m_commandBufferUAVs[frameIndex].GetResource(),
		m_commandBufferUAVs[frameIndex].GetCounterOffset(0u),
		m_uavCounterBuffer.GetResource(), 0u, static_cast<UINT64>(sizeof(UINT))
	); // UAV getting promoted from COMMON to COPY_DEST

	// So need to change it back to UNORDERED_ACCESS
	D3D12_RESOURCE_BARRIER indirectArgsBarrier = GetTransitionBarrier(
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		frameIndex
	);
	commandList->ResourceBarrier(1u, &indirectArgsBarrier);
}
