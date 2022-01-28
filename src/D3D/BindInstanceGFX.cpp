#include <BindInstanceGFX.hpp>
#include <InstanceManager.hpp>
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

	size_t vertexOffset = VertexBufferInst::GetRef()->AddData(
		modelRef->GetVertexData(), vertexBufferSize
	);
	size_t indexOffset = IndexBufferInst::GetRef()->AddData(
		modelRef->GetIndexData(), indexBufferSize
	);

	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			D3D12_VERTEX_BUFFER_VIEW{
				vertexOffset,
				static_cast<UINT>(vertexBufferSize),
				static_cast<UINT>(modelRef->GetVertexStrideSize())
			},
			D3D12_INDEX_BUFFER_VIEW{
				indexOffset,
				static_cast<UINT>(indexBufferSize),
				DXGI_FORMAT_R16_UINT
			},
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

void BindInstanceGFX::UpldateBufferViewAddresses(
	size_t vertexAddress,
	size_t indexAddress
) noexcept {
	for (size_t index = 0u; index < m_modelsRaw.size(); ++index) {
		m_modelsRaw[index]->UpdateVBVGPUOffset(vertexAddress);
		m_modelsRaw[index]->UpdateIBVGPUOffset(indexAddress);
	}
}

// Model Raw
BindInstanceGFX::ModelRaw::ModelRaw() noexcept
	:
	m_vertexBufferView{},
	m_indexBufferView{},
	m_indexCount(0u) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
	const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
	size_t indicesCount
) noexcept
	:
	m_vertexBufferView(vertexBufferView),
	m_indexBufferView(indexBufferView),
	m_indexCount(static_cast<UINT>(indicesCount)) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView,
	D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
	size_t indicesCount
) noexcept
	:
	m_vertexBufferView(std::move(vertexBufferView)),
	m_indexBufferView(std::move(indexBufferView)),
	m_indexCount(static_cast<UINT>(indicesCount)) {}

void BindInstanceGFX::ModelRaw::AddVBV(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView) noexcept {
	m_vertexBufferView = vertexBufferView;
}

void BindInstanceGFX::ModelRaw::AddVBV(D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView) noexcept {
	m_vertexBufferView = std::move(vertexBufferView);
}

void BindInstanceGFX::ModelRaw::AddIBV(
	const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
	size_t indicesCount
) noexcept {
	m_indexBufferView = indexBufferView;
	m_indexCount = static_cast<UINT>(indicesCount);
}

void BindInstanceGFX::ModelRaw::AddIBV(
	D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
	size_t indicesCount
) noexcept {
	m_indexBufferView = std::move(indexBufferView);
	m_indexCount = static_cast<UINT>(indicesCount);
}

void BindInstanceGFX::ModelRaw::Draw(ID3D12GraphicsCommandList* commandList) noexcept {
	commandList->IASetVertexBuffers(0u, 1u, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);

	commandList->DrawIndexedInstanced(m_indexCount, 1u, 0u, 0u, 0u);
}

void BindInstanceGFX::ModelRaw::UpdateVBVGPUOffset(size_t offset) noexcept {
	m_vertexBufferView.BufferLocation += offset;
}

void BindInstanceGFX::ModelRaw::UpdateIBVGPUOffset(size_t offset) noexcept {
	m_indexBufferView.BufferLocation += offset;
}
