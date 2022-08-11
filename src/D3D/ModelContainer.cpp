#include <ModelContainer.hpp>
#include <Shader.hpp>
#include <Gaia.hpp>
#include <VertexLayout.hpp>

ModelContainer::ModelContainer(
	const std::string& shaderPath, std::uint32_t bufferCount
) noexcept
	: m_renderPipeline(std::make_unique<RenderPipeline>(bufferCount)),
	m_pPerFrameBuffers(std::make_unique<PerFrameBuffers>(bufferCount)),
	m_shaderPath(shaderPath) {}

void ModelContainer::AddModels(std::vector<std::shared_ptr<IModel>>&& models) {
	m_renderPipeline->AddOpaqueModels(std::move(models));
}

void ModelContainer::AddModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	m_pPerFrameBuffers->AddModelInputs(
		std::move(vertices), vertexBufferSize, strideSize,
		std::move(indices), indexBufferSize
	);
}

void ModelContainer::BindCommands(
	ID3D12GraphicsCommandList* commandList, size_t frameIndex
) const noexcept {
	m_renderPipeline->BindGraphicsPipeline(commandList);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = Gaia::descriptorTable->GetTextureRangeStart();
	commandList->SetGraphicsRootDescriptorTable(0u, gpuHandle);

	m_pPerFrameBuffers->BindPerFrameBuffers(commandList, frameIndex);
	m_renderPipeline->UpdateModels(frameIndex);
	m_renderPipeline->DrawModels(commandList, frameIndex);
}

void ModelContainer::CopyData(std::atomic_size_t& workCount) {
	workCount += 2u;

	Gaia::threadPool->SubmitWork(
		[&workCount] {
			Gaia::vertexBuffer->CopyData();

			--workCount;
		}
	);

	Gaia::threadPool->SubmitWork(
		[&workCount] {
			Gaia::indexBuffer->CopyData();

			--workCount;
		}
	);
}

void ModelContainer::RecordUploadBuffers(ID3D12GraphicsCommandList* copyList) {
	Gaia::heapManager->RecordUpload(copyList);
}

void ModelContainer::ReserveBuffers(ID3D12Device* device) {
	m_renderPipeline->ReserveCommandBuffers(device);

	// Acquire all buffers first
	Gaia::vertexBuffer->AcquireBuffers();
	Gaia::indexBuffer->AcquireBuffers();

	Gaia::heapManager->ReserveHeapSpace();
	Gaia::cpuWriteBuffer->ReserveHeapSpace(device);
}

void ModelContainer::CreateBuffers(ID3D12Device* device) {
	// Now allocate memory and actually create them
	Gaia::heapManager->CreateBuffers(device);

	Gaia::cpuWriteBuffer->CreateResource(device);
	m_renderPipeline->CreateCommandBuffers(device);

	// Set GPU addresses
	Gaia::vertexBuffer->SetGPUVirtualAddressToBuffers();
	Gaia::indexBuffer->SetGPUVirtualAddressToBuffers();

	m_pPerFrameBuffers->SetMemoryAddresses();
}

void ModelContainer::ReleaseUploadBuffers() {
	Gaia::vertexBuffer->ReleaseUploadBuffer();
	Gaia::indexBuffer->ReleaseUploadBuffer();

	Gaia::heapManager->ReleaseUploadBuffer();
}

void ModelContainer::InitPipelines(ID3D12Device* device) {
	auto [pso, signature] = CreatePipeline(device);

	m_renderPipeline->AddGraphicsRootSignature(std::move(signature));
	m_renderPipeline->AddGraphicsPipelineObject(std::move(pso));

	m_renderPipeline->CreateCommandSignature(device);
}

ModelContainer::Pipeline ModelContainer::CreatePipeline(ID3D12Device* device) const {
	std::unique_ptr<RootSignatureDynamic> signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, D3D12_SHADER_VISIBILITY_PIXEL, false, true,
		0u
	);
	signature->AddShaderResourceView(D3D12_SHADER_VISIBILITY_VERTEX, true, 0u);
	signature->AddConstants(1u, D3D12_SHADER_VISIBILITY_VERTEX, 2u);
	signature->AddShaderResourceView(D3D12_SHADER_VISIBILITY_VERTEX, true, 1u);

	signature->CompileSignature();
	signature->CreateSignature(device);

	std::unique_ptr<Shader> vs = std::make_unique<Shader>();
	vs->LoadBinary(m_shaderPath + "VertexShader.cso");

	std::unique_ptr<Shader> ps = std::make_unique<Shader>();
	ps->LoadBinary(m_shaderPath + "PixelShader.cso");

	VertexLayout vertexLayout{};
	vertexLayout.AddInputElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 12u);
	vertexLayout.AddInputElement("UV", DXGI_FORMAT_R32G32_FLOAT, 8u);

	std::unique_ptr<PipelineObjectGFX> pso = std::make_unique<PipelineObjectGFX>();
	pso->CreatePipelineState(
		device,
		vertexLayout.GetLayoutDesc(),
		signature->Get(),
		vs->GetByteCode(),
		ps->GetByteCode()
	);

	return {
		std::move(pso),
		std::move(signature)
	};
}
