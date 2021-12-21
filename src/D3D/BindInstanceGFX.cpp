#include <BindInstanceGFX.hpp>
#include <VertexBuffer.hpp>
#include <IndexBuffer.hpp>

BindInstanceGFX::BindInstanceGFX(
	std::unique_ptr<IPipelineObject> pso,
	std::shared_ptr<IRootSignature> signature
) noexcept : m_pso(std::move(pso)), m_rootSignature(signature) {}

void BindInstanceGFX::AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept {
	m_pso = std::move(pso);
}

void BindInstanceGFX::AddRootSignature(std::shared_ptr<IRootSignature> signature) noexcept {
	m_rootSignature = signature;
}

void BindInstanceGFX::AddColoredModel(
	ID3D12Device* device, const IModel* const modelRef
) noexcept {
	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			std::make_unique<VertexBuffer>(
				device,
				modelRef->GetVertices(),
				modelRef->GetSolidColor()
				),
			std::make_unique<IndexBuffer>(device, modelRef->GetIndices()),
			modelRef
			)
	);
}

void BindInstanceGFX::AddTexturedModel(
	ID3D12Device* device, const IModel* const modelRef
) noexcept {
	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			std::make_unique<VertexBuffer>(
				device,
				modelRef->GetVertices()
				),
			std::make_unique<IndexBuffer>(device, modelRef->GetIndices()),
			modelRef
			)
	);
}

void BindInstanceGFX::BindCommands(ID3D12GraphicsCommandList* commandList) noexcept {
	commandList->SetPipelineState(m_pso->Get());
	commandList->SetGraphicsRootSignature(m_rootSignature->Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& model : m_modelsRaw)
		model->Draw(commandList);
}

// Model Raw
BindInstanceGFX::ModelRaw::ModelRaw(const IModel* const modelRef) noexcept
	: m_modelRef(modelRef) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	std::unique_ptr<IVertexBuffer> vertexBuffer,
	std::unique_ptr<IIndexBuffer> indexBuffer,
	const IModel* const modelRef
) noexcept
	:
	m_vertexBuffer(std::move(vertexBuffer)),
	m_indexBuffer(std::move(indexBuffer)), m_modelRef(modelRef) {}

void BindInstanceGFX::ModelRaw::AddVB(std::unique_ptr<IVertexBuffer> vertexBuffer) noexcept {
	m_vertexBuffer = std::move(vertexBuffer);
}
void BindInstanceGFX::ModelRaw::AddIB(std::unique_ptr<IIndexBuffer> indexBuffer) noexcept {
	m_indexBuffer = std::move(indexBuffer);
}

void BindInstanceGFX::ModelRaw::Draw(ID3D12GraphicsCommandList* commandList) noexcept {
	commandList->IASetVertexBuffers(0u, 1u, m_vertexBuffer->GetVertexBufferRef());
	commandList->IASetIndexBuffer(m_indexBuffer->GetIndexBufferRef());

	commandList->DrawIndexedInstanced(m_indexBuffer->GetIndexCount(), 1u, 0u, 0u, 0u);
}

