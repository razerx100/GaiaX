#include <RenderPipelineIndirectDraw.hpp>
#include <ranges>
#include <algorithm>
#include <array>
#include <Gaia.hpp>
#include <D3DResourceBarrier.hpp>
#include <VertexLayout.hpp>
#include <Shader.hpp>

RenderPipelineIndirectDraw::RenderPipelineIndirectDraw(std::uint32_t frameCount) noexcept
	: m_modelCount(0u), m_frameCount{ frameCount },
	m_commandBufferUAVs{
		frameCount,
		{ ResourceType::gpuOnly, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS }
	}, m_uavCounterBuffer{ ResourceType::cpuWrite } {}

void RenderPipelineIndirectDraw::BindGraphicsPipeline(
	ID3D12GraphicsCommandList* graphicsCommandList, ID3D12RootSignature* graphicsRS
) const noexcept {
	graphicsCommandList->SetPipelineState(m_graphicPSO->Get());
	graphicsCommandList->SetGraphicsRootSignature(graphicsRS);
	graphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void RenderPipelineIndirectDraw::DrawModels(
	ID3D12CommandSignature* commandSignature,
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) const noexcept {
	graphicsCommandList->ExecuteIndirect(
		commandSignature, m_modelCount, m_commandBufferUAVs[frameIndex].GetResource(),
		m_commandBufferUAVs[frameIndex].GetFirstBufferOffset(),
		m_commandBufferUAVs[frameIndex].GetResource(),
		m_commandBufferUAVs[frameIndex].GetFirstCounterOffset()
	);
}

void RenderPipelineIndirectDraw::ReserveBuffers(ID3D12Device* device) {
	const size_t commandDescriptorOffsetSRV =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();
	size_t commandDescriptorOffsetUAV =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);

	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	static constexpr auto indirectStructSize = static_cast<UINT>(sizeof(IndirectCommand));

	m_commandBufferSRV.SetDescriptorOffset(commandDescriptorOffsetSRV, descriptorSize);
	m_commandBufferSRV.SetBufferInfo(device, indirectStructSize, m_modelCount, 1u);

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

void RenderPipelineIndirectDraw::RecordIndirectArguments(
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

void RenderPipelineIndirectDraw::CreateBuffers(ID3D12Device* device) {
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
	std::uint8_t* commandCPUPtr = m_commandBufferSRV.GetFirstBufferCPUWPointer();
	memcpy(
		commandCPUPtr, std::data(m_indirectCommands),
		sizeof(IndirectCommand) * std::size(m_indirectCommands)
	);

	m_indirectCommands = std::vector<IndirectCommand>();
}

void RenderPipelineIndirectDraw::DispatchCompute(
	ID3D12GraphicsCommandList* computeCommandList, size_t frameIndex,
	const RSLayoutType& computeLayout
) const noexcept {
	static constexpr size_t commandBufferSRVIndex =
		static_cast<size_t>(RootSigElement::IndirectArgsSRV);
	static constexpr size_t commandBufferUAVIndex =
		static_cast<size_t>(RootSigElement::IndirectArgsUAV);
	static constexpr size_t cullingDataIndex =
		static_cast<size_t>(RootSigElement::CullingData);

	computeCommandList->SetComputeRootDescriptorTable(
		computeLayout[commandBufferSRVIndex],
		m_commandBufferSRV.GetFirstGPUDescriptorHandle()
	);
	computeCommandList->SetComputeRootDescriptorTable(
		computeLayout[commandBufferUAVIndex],
		m_commandBufferUAVs[frameIndex].GetFirstGPUDescriptorHandle()
	);
	computeCommandList->SetComputeRootConstantBufferView(
		computeLayout[cullingDataIndex], m_cullingDataBuffer.GetGPUAddress()
	);

	computeCommandList->Dispatch(
		static_cast<UINT>(std::ceil(m_modelCount / THREADBLOCKSIZE)), 1u, 1u
	);
}

void RenderPipelineIndirectDraw::RecordResourceUpload(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_commandBufferSRV.RecordResourceUpload(copyList);
	m_cullingDataBuffer.RecordResourceUpload(copyList);
}

void RenderPipelineIndirectDraw::ReleaseUploadResource() noexcept {
	m_commandBufferSRV.ReleaseUploadResource();
	m_cullingDataBuffer.ReleaseUploadResource();
}

ID3D12Resource* RenderPipelineIndirectDraw::GetArgumentBuffer(
	size_t frameIndex
) const noexcept {
	return m_commandBufferUAVs[frameIndex].GetResource();
}

void RenderPipelineIndirectDraw::ResetCounterBuffer(
	ID3D12GraphicsCommandList* commandList, size_t frameIndex
) const noexcept {
	commandList->CopyBufferRegion(
		m_commandBufferUAVs[frameIndex].GetResource(),
		m_commandBufferUAVs[frameIndex].GetFirstCounterOffset(),
		m_uavCounterBuffer.GetResource(), 0u, static_cast<UINT64>(sizeof(UINT))
	); // UAV getting promoted from COMMON to COPY_DEST

	// So need to change it back to UNORDERED_ACCESS
	D3DResourceBarrier().AddBarrier(
		m_commandBufferUAVs[frameIndex].GetResource(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	).RecordBarriers(commandList);
}

std::unique_ptr<D3DPipelineObject> RenderPipelineIndirectDraw::CreateGraphicsPipelineObject(
	ID3D12Device* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
	ID3D12RootSignature* graphicsRootSignature
) const noexcept {
	auto vs = std::make_unique<Shader>();
	vs->LoadBinary(shaderPath + L"VertexShader.cso");

	auto ps = std::make_unique<Shader>();
	ps->LoadBinary(shaderPath + pixelShader);

	auto pso = std::make_unique<D3DPipelineObject>();
	pso->CreateGFXPipelineState(
		device,
		VertexLayout()
		.AddInputElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 12u)
		.AddInputElement("UV", DXGI_FORMAT_R32G32_FLOAT, 8u),
		graphicsRootSignature, vs->GetByteCode(), ps->GetByteCode()
	);

	return pso;
}

void RenderPipelineIndirectDraw::ConfigureGraphicsPipelineObject(
	ID3D12Device* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
	ID3D12RootSignature* graphicsRootSignature
) noexcept {
	m_graphicPSO = CreateGraphicsPipelineObject(
		device, shaderPath, pixelShader, graphicsRootSignature
	);
}