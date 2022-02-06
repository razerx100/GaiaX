#include <BindInstanceGFX.hpp>
#include <InstanceManager.hpp>
#include <IndexBufferView.hpp>
#include <VertexBufferView.hpp>
#include <CRSMath.hpp>

BindInstanceGFX::BindInstanceGFX(
bool textureAvailable
) noexcept : m_textureAvailable(textureAvailable) {}

BindInstanceGFX::BindInstanceGFX(
	bool textureAvailable,
	std::unique_ptr<IPipelineObject> pso,
	std::unique_ptr<IRootSignature> signature
) noexcept
	: m_pso(std::move(pso)),
	m_rootSignature(std::move(signature)), m_textureAvailable(textureAvailable) {}

void BindInstanceGFX::AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept {
	m_pso = std::move(pso);
}

void BindInstanceGFX::AddRootSignature(std::unique_ptr<IRootSignature> signature) noexcept {
	m_rootSignature = std::move(signature);
}

void BindInstanceGFX::AddModel(
	const IModel* const modelRef
) noexcept {
	size_t indexBufferSize = modelRef->GetIndexBufferSize();
	size_t vertexBufferSize = modelRef->GetVertexBufferSize();

	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			modelRef,
			std::make_unique<VertexBufferView>(
				VertexBufferInst::GetRef()->AddBuffer(
					modelRef->GetVertexData(), vertexBufferSize, BufferType::Vertex
				),
				vertexBufferSize,
				modelRef->GetVertexStrideSize()
				)
			,
			std::make_unique<IndexBufferView>(
				IndexBufferInst::GetRef()->AddBuffer(
					modelRef->GetIndexData(), indexBufferSize, BufferType::Index
				),
				indexBufferSize
				),
			modelRef->GetIndexCount()
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
	:
	m_modelRef(modelRef),
	m_indexCount(0u) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	const IModel* const modelRef,
	std::unique_ptr<IVertexBufferView> vertexBuffer,
	std::unique_ptr<IIndexBufferView> indexBuffer,
	size_t indexCount
) noexcept
	:
	m_modelRef(modelRef),
	m_vertexBuffer(std::move(vertexBuffer)),
	m_indexBuffer(std::move(indexBuffer)),
	m_indexCount(static_cast<UINT>(indexCount)) {}

void BindInstanceGFX::ModelRaw::AddVB(
	std::unique_ptr<IVertexBufferView> vertexBuffer
) noexcept {
	m_vertexBuffer = std::move(vertexBuffer);
}

void BindInstanceGFX::ModelRaw::AddIB(
	std::unique_ptr<IIndexBufferView> indexBuffer,
	size_t indexCount
) noexcept {
	m_indexBuffer = std::move(indexBuffer);
	m_indexCount = static_cast<UINT>(indexCount);
}

void BindInstanceGFX::ModelRaw::Draw(ID3D12GraphicsCommandList* commandList) noexcept {
	commandList->IASetVertexBuffers(0u, 1u, m_vertexBuffer->GetBufferViewRef());
	commandList->IASetIndexBuffer(m_indexBuffer->GetBufferViewRef());

	commandList->DrawIndexedInstanced(m_indexCount, 1u, 0u, 0u, 0u);
}
