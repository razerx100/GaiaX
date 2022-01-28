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
	ID3D12Device* device, const IModel* const modelRef, bool texture
) {
	if (texture)
		AddTexturedModel(device, modelRef);
	else
		AddColoredModel(device, modelRef);
}

void ModelContainer::AddColoredModel(
	ID3D12Device* device, const IModel* const modelRef
) {
	if (!m_coloredInstanceData.available) {
		InitNewInstance(m_coloredInstanceData, false);

		std::unique_ptr<RootSignatureDynamic> signature =
			std::make_unique<RootSignatureDynamic>();
		signature->CompileSignature(false);
		signature->CreateSignature(device);

		VertexLayout layout = VertexLayout(modelRef->GetVertexLayout());

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

		m_bindInstances[m_coloredInstanceData.index]->AddRootSignature(std::move(signature));

		m_bindInstances[m_coloredInstanceData.index]->AddPSO(std::move(pso));
	}

	m_bindInstances[m_coloredInstanceData.index]->AddModel(modelRef);
}

void ModelContainer::AddTexturedModel(
	ID3D12Device* device, const IModel* const modelRef
) {
	if (!m_texturedInstanceData.available) {
		InitNewInstance(m_texturedInstanceData, true);

		// Init Pipeline Object
	}

	m_bindInstances[m_texturedInstanceData.index]->AddModel(modelRef);
}

void ModelContainer::BindCommands(ID3D12GraphicsCommandList* commandList) noexcept {
	for (auto& bindInstance : m_bindInstances)
		bindInstance->BindCommands(commandList);
}

void ModelContainer::InitNewInstance(InstanceData& instanceData, bool texture) noexcept {
	m_bindInstances.emplace_back(std::make_unique<BindInstanceGFX>(texture));
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
	VertexBufferInst::GetRef()->RecordUpload(copyList);
	IndexBufferInst::GetRef()->RecordUpload(copyList);
}

void ModelContainer::CreateBuffers(ID3D12Device* device) {
	IResourceBuffer* vertexBufferRef = VertexBufferInst::GetRef();
	IResourceBuffer* indexBufferRef = IndexBufferInst::GetRef();

	vertexBufferRef->CreateBuffer(device);
	indexBufferRef->CreateBuffer(device);

	for (auto& bindInstance : m_bindInstances)
		bindInstance->UpldateBufferViewAddresses(
			static_cast<size_t>(vertexBufferRef->GetGPUHandle()),
			static_cast<size_t>(indexBufferRef->GetGPUHandle())
		);
}

void ModelContainer::ReleaseUploadBuffers() {
	VertexBufferInst::GetRef()->ReleaseUploadBuffer();
	IndexBufferInst::GetRef()->ReleaseUploadBuffer();
}
