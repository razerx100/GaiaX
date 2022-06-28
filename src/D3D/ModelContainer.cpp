#include <ModelContainer.hpp>
#include <BindInstanceGFX.hpp>
#include <RootSignatureDynamic.hpp>
#include <Shader.hpp>
#include <PipelineObjectGFX.hpp>
#include <Gaia.hpp>

ModelContainer::ModelContainer(const std::string& shaderPath) noexcept
	: m_bindInstance(std::make_unique<BindInstanceGFX>()),
	m_pPerFrameBuffers(std::make_unique<PerFrameBuffers>()),
	m_shaderPath(shaderPath) {}

void ModelContainer::AddModel(std::shared_ptr<IModel>&& model) {
	m_bindInstance->AddModel(std::move(model));
}

void ModelContainer::BindCommands(ID3D12GraphicsCommandList* commandList) const noexcept {
	m_bindInstance->BindPipelineObjects(commandList);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = Gaia::descriptorTable->GetTextureRangeStart();
	commandList->SetGraphicsRootDescriptorTable(1u, gpuHandle);

	m_pPerFrameBuffers->BindPerFrameBuffers(commandList);

	m_bindInstance->BindModels(commandList);
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

void ModelContainer::CreateBuffers(ID3D12Device* device) {
	// Acquire all buffers first
	Gaia::vertexBuffer->AcquireBuffers();
	Gaia::indexBuffer->AcquireBuffers();

	// Now allocate memory and actually create them
	Gaia::heapManager->CreateBuffers(device);
	Gaia::constantBuffer->CreateBuffer(device);

	// Set GPU addresses
	Gaia::vertexBuffer->SetGPUVirtualAddressToBuffers();
	Gaia::indexBuffer->SetGPUVirtualAddressToBuffers();

	m_pPerFrameBuffers->SetMemoryAddresses();
	m_bindInstance->SetGPUVirtualAddresses();
}

void ModelContainer::ReleaseUploadBuffers() {
	Gaia::vertexBuffer->ReleaseUploadBuffer();
	Gaia::indexBuffer->ReleaseUploadBuffer();

	Gaia::heapManager->ReleaseUploadBuffer();
}

void ModelContainer::InitPipelines(ID3D12Device* device) {
	auto [pso, signature] = CreatePipeline(
		device,
		m_bindInstance->GetVertexLayout()
	);

	m_bindInstance->AddRootSignature(std::move(signature));
	m_bindInstance->AddPSO(std::move(pso));
}

ModelContainer::Pipeline ModelContainer::CreatePipeline(
	ID3D12Device* device, const VertexLayout& layout
) const {
	std::unique_ptr<RootSignatureDynamic> signature = std::make_unique<RootSignatureDynamic>();

	signature->AddConstants(1u, D3D12_SHADER_VISIBILITY_PIXEL, 0u);
	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		static_cast<std::uint32_t>(Gaia::descriptorTable->GetTextureDescriptorCount()),
		D3D12_SHADER_VISIBILITY_PIXEL, 0u
	);
	signature->AddConstants(2u, D3D12_SHADER_VISIBILITY_VERTEX, 1u);
	signature->AddConstants(16u, D3D12_SHADER_VISIBILITY_VERTEX, 2u);
	signature->AddConstantBufferView(D3D12_SHADER_VISIBILITY_VERTEX, 3u);

	signature->CompileSignature();
	signature->CreateSignature(device);

	std::unique_ptr<Shader> vs = std::make_unique<Shader>();
	vs->LoadBinary(m_shaderPath + "VertexShader.cso");

	std::unique_ptr<Shader> ps = std::make_unique<Shader>();
	ps->LoadBinary(m_shaderPath + "PixelShader.cso");

	std::unique_ptr<PipelineObjectGFX> pso = std::make_unique<PipelineObjectGFX>();
	pso->CreatePipelineState(
		device,
		layout,
		signature->Get(),
		vs->GetByteCode(),
		ps->GetByteCode()
	);

	return {
		std::move(pso),
		std::move(signature)
	};
}
