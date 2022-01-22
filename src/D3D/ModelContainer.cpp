#include <ModelContainer.hpp>
#include <BindInstanceGFX.hpp>
#include <RootSignatureDynamic.hpp>
#include <Shader.hpp>
#include <PipelineObjectGFX.hpp>
#include <VertexLayout.hpp>

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

		if (!m_pRootSignature)
			CreateRootSignature(device);

		m_bindInstances[m_coloredInstanceData.index]->AddRootSignature(m_pRootSignature);

		VertexLayout layout = VertexLayout(modelRef->GetVertexLayout());

		std::shared_ptr<Shader> vs = std::make_shared<Shader>();
		vs->LoadBinary(m_shaderPath + "VSColored.cso");

		std::shared_ptr<Shader> ps = std::make_shared<Shader>();
		ps->LoadBinary(m_shaderPath + "PSColored.cso");

		std::unique_ptr<PipelineObjectGFX> pso = std::make_unique<PipelineObjectGFX>();
		pso->CreatePipelineState(
			device,
			layout,
			m_pRootSignature->Get(),
			vs,
			ps
		);

		m_bindInstances[m_coloredInstanceData.index]->AddPSO(std::move(pso));
	}

	m_bindInstances[m_coloredInstanceData.index]->AddModel(modelRef);
}

void ModelContainer::AddTexturedModel(
	ID3D12Device* device, const IModel* const modelRef
) {
	if (!m_texturedInstanceData.available) {
		InitNewInstance(m_texturedInstanceData, true);

		if (!m_pRootSignature)
			CreateRootSignature(device);

		m_bindInstances[m_texturedInstanceData.index]->AddRootSignature(m_pRootSignature);

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

void ModelContainer::CreateRootSignature(ID3D12Device* device) {
	std::shared_ptr<RootSignatureDynamic> signature = std::make_shared<RootSignatureDynamic>();
	signature->CompileSignature(false);
	signature->CreateSignature(device);

	m_pRootSignature = signature;
}

void ModelContainer::CopyBuffers(ID3D12Device* device) {
	for (auto& bindInstance : m_bindInstances)
		bindInstance->CopyData(device);
}

void ModelContainer::RecordUploadBuffers(ID3D12GraphicsCommandList* copyList) {
	for (auto& bindInstance : m_bindInstances)
		bindInstance->RecordUploadBuffers(copyList);
}
