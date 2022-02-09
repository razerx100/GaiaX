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

	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			modelRef,
			D3D12_VERTEX_BUFFER_VIEW{
				VertexBufferInst::GetRef()->AddDataAndGetOffset(
					modelRef->GetVertexData(), vertexBufferSize
				),
				static_cast<UINT>(vertexBufferSize),
				static_cast<UINT>(modelRef->GetVertexStrideSize())
			},
			D3D12_INDEX_BUFFER_VIEW{
				IndexBufferInst::GetRef()->AddDataAndGetOffset(
					modelRef->GetIndexData(), indexBufferSize
				),
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

void BindInstanceGFX::SetGPUVirtualAddresses() noexcept {
	D3D12_GPU_VIRTUAL_ADDRESS vertexAddress =
		VertexBufferInst::GetRef()->GetGPUVirtualAddress();

	D3D12_GPU_VIRTUAL_ADDRESS indexAddress =
		IndexBufferInst::GetRef()->GetGPUVirtualAddress();

	for (auto& model : m_modelsRaw)
		model->UpdateGPUAddressOffsets(
			vertexAddress,
			indexAddress
		);
}

// Model Raw
BindInstanceGFX::ModelRaw::ModelRaw(const IModel* const modelRef) noexcept
	:
	m_modelRef(modelRef),
	m_indexCount(0u),
	m_indexBufferView{}, m_vertexBufferView{} {}

BindInstanceGFX::ModelRaw::ModelRaw(
	const IModel* const modelRef,
	const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
	const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
	size_t indexCount
) noexcept
	:
	m_modelRef(modelRef),
	m_vertexBufferView(vertexBufferView),
	m_indexBufferView(indexBufferView),
	m_indexCount(static_cast<UINT>(indexCount)) {}

void BindInstanceGFX::ModelRaw::AddVB(
	const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView
) noexcept {
	m_vertexBufferView = vertexBufferView;
}

void BindInstanceGFX::ModelRaw::AddIB(
	const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
	size_t indexCount
) noexcept {
	m_indexBufferView = indexBufferView;
	m_indexCount = static_cast<UINT>(indexCount);
}

void BindInstanceGFX::ModelRaw::UpdateGPUAddressOffsets(
	D3D12_GPU_VIRTUAL_ADDRESS vbvAddress,
	D3D12_GPU_VIRTUAL_ADDRESS ibvAddress
) {
	m_indexBufferView.BufferLocation += ibvAddress;
	m_vertexBufferView.BufferLocation += vbvAddress;
}

void BindInstanceGFX::ModelRaw::Draw(ID3D12GraphicsCommandList* commandList) noexcept {
	commandList->IASetVertexBuffers(0u, 1u, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);

	commandList->DrawIndexedInstanced(m_indexCount, 1u, 0u, 0u, 0u);
}
