#include <ModelContainer.hpp>
#include <BindInstanceGFX.hpp>
#include <RootSignatureDynamic.hpp>
#include <Shader.hpp>
#include <PipelineObjectGFX.hpp>
#include <InstanceManager.hpp>
#include <VenusInstance.hpp>

ModelContainer::ModelContainer(const char* shaderPath) noexcept
	: m_bindInstance(std::make_unique<BindInstanceGFX>()), m_shaderPath(shaderPath) {}

void ModelContainer::AddModel(
	const IModel* const modelRef
) {
	m_bindInstance->AddModel(modelRef);
}

void ModelContainer::BindCommands(ID3D12GraphicsCommandList* commandList) noexcept {
	m_bindInstance->BindCommands(commandList);
}

void ModelContainer::CopyData(std::atomic_size_t& workCount) {
	workCount += 2u;

	GetVenusInstance()->SubmitWork(
		[&workCount] {
			VertexBufferInst::GetRef()->CopyData();

			--workCount;
		}
	);

	GetVenusInstance()->SubmitWork(
		[&workCount] {
			IndexBufferInst::GetRef()->CopyData();

			--workCount;
		}
	);
}

void ModelContainer::RecordUploadBuffers(ID3D12GraphicsCommandList* copyList) {
	HeapManagerInst::GetRef()->RecordUpload(copyList);
}

void ModelContainer::CreateBuffers(ID3D12Device* device) {
	// Acquire all buffers first
	VertexBufferInst::GetRef()->AcquireBuffers();
	IndexBufferInst::GetRef()->AcquireBuffers();

	// Now allocate memory and actually create them
	HeapManagerInst::GetRef()->CreateBuffers(device);

	// Set GPU addresses
	VertexBufferInst::GetRef()->SetGPUVirtualAddressToBuffers();
	IndexBufferInst::GetRef()->SetGPUVirtualAddressToBuffers();

	m_bindInstance->SetGPUVirtualAddresses();
}

void ModelContainer::ReleaseUploadBuffers() {
	VertexBufferInst::GetRef()->ReleaseUploadBuffer();
	IndexBufferInst::GetRef()->ReleaseUploadBuffer();

	HeapManagerInst::GetRef()->ReleaseUploadBuffer();
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
	std::unique_ptr<RootSignatureDynamic> signature =
		std::make_unique<RootSignatureDynamic>();
	signature->CompileSignature(false);
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
