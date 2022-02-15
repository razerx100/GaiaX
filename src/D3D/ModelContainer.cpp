#include <ModelContainer.hpp>
#include <BindInstanceGFX.hpp>
#include <RootSignatureDynamic.hpp>
#include <Shader.hpp>
#include <PipelineObjectGFX.hpp>
#include <VertexLayout.hpp>
#include <InstanceManager.hpp>
#include <VenusInstance.hpp>

ModelContainer::ModelContainer(const char* shaderPath) noexcept
	: m_coloredInstanceData{}, m_texturedInstanceData{}, m_shaderPath(shaderPath) {}

void ModelContainer::AddModel(
	const IModel* const modelRef, bool texture
) {
	if (texture)
		AddTexturedModel(modelRef);
	else
		AddColoredModel(modelRef);
}

void ModelContainer::AddColoredModel(
	const IModel* const modelRef
) {
	if (!m_coloredInstanceData.available)
		InitNewInstance(m_coloredInstanceData);

	m_bindInstances[m_coloredInstanceData.index]->AddModel(modelRef);
}

void ModelContainer::AddTexturedModel(
	const IModel* const modelRef
) {
	if (!m_texturedInstanceData.available)
		InitNewInstance(m_texturedInstanceData);

	m_bindInstances[m_texturedInstanceData.index]->AddModel(modelRef);
}

void ModelContainer::BindCommands(ID3D12GraphicsCommandList* commandList) noexcept {
	for (auto& bindInstance : m_bindInstances)
		bindInstance->BindCommands(commandList);
}

void ModelContainer::InitNewInstance(InstanceData& instanceData) noexcept {
	m_bindInstances.emplace_back(std::make_unique<BindInstanceGFX>());
	instanceData = { true, m_bindInstances.size() - 1u };
}

void ModelContainer::CopyData() {
	std::atomic_size_t works = 2u;

	GetVenusInstance()->SubmitWork(
		[&works] {
			VertexBufferInst::GetRef()->CopyData();

			--works;
		}
	);

	GetVenusInstance()->SubmitWork(
		[&works] {
			IndexBufferInst::GetRef()->CopyData();

			--works;
		}
	);

	while (works != 0u);
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

	for (auto& bindInstance : m_bindInstances)
		bindInstance->SetGPUVirtualAddresses();
}

void ModelContainer::ReleaseUploadBuffers() {
	VertexBufferInst::GetRef()->ReleaseUploadBuffer();
	IndexBufferInst::GetRef()->ReleaseUploadBuffer();

	HeapManagerInst::GetRef()->ReleaseUploadBuffer();
}

void ModelContainer::InitPipelines(ID3D12Device* device) {
	if (m_coloredInstanceData.available) {
		size_t coloredIndex = m_coloredInstanceData.index;

		auto [pso, signature] = CreateColoredPipeline(
			device,
			m_bindInstances[coloredIndex]->GetVertexLayout()
		);

		m_bindInstances[coloredIndex]->AddRootSignature(std::move(signature));
		m_bindInstances[coloredIndex]->AddPSO(std::move(pso));
	}

	if (m_texturedInstanceData.available) {
		size_t texturedIndex = m_texturedInstanceData.index;

		auto [pso, signature] = CreateColoredPipeline(
			device,
			m_bindInstances[texturedIndex]->GetVertexLayout()
		);

		m_bindInstances[texturedIndex]->AddRootSignature(std::move(signature));
		m_bindInstances[texturedIndex]->AddPSO(std::move(pso));
	}
}

ModelContainer::Pipeline ModelContainer::CreateColoredPipeline(
	ID3D12Device* device, const VertexLayout& layout
) const {
	std::unique_ptr<RootSignatureDynamic> signature =
		std::make_unique<RootSignatureDynamic>();
	signature->CompileSignature(false);
	signature->CreateSignature(device);

	std::unique_ptr<Shader> vs = std::make_unique<Shader>();
	vs->LoadBinary(m_shaderPath + "VSColored.cso");

	std::unique_ptr<Shader> ps = std::make_unique<Shader>();
	ps->LoadBinary(m_shaderPath + "PSColored.cso");

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

ModelContainer::Pipeline ModelContainer::CreateTexturedPipeline(
	ID3D12Device* device, const VertexLayout& layout
) const {
	return {
		std::make_unique<PipelineObjectGFX>(),
		std::make_unique<RootSignatureDynamic>()
	};
}
