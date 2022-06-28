#include <BindInstanceGFX.hpp>
#include <Gaia.hpp>

#include <DirectXMath.h>

BindInstanceGFX::BindInstanceGFX() noexcept : m_vertexLayout() {}

BindInstanceGFX::BindInstanceGFX(
	std::unique_ptr<PipelineObjectGFX> pso, std::unique_ptr<RootSignatureDynamic> signature
) noexcept
	: m_pso(std::move(pso)),
	m_rootSignature(std::move(signature)), m_vertexLayout() {}

void BindInstanceGFX::AddPSO(std::unique_ptr<PipelineObjectGFX> pso) noexcept {
	m_pso = std::move(pso);
}

void BindInstanceGFX::AddRootSignature(
	std::unique_ptr<RootSignatureDynamic> signature
) noexcept {
	m_rootSignature = std::move(signature);
}

void BindInstanceGFX::AddModel(std::shared_ptr<IModel>&& model) noexcept {
	size_t indexBufferSize = model->GetIndexBufferSize();
	size_t vertexBufferSize = model->GetVertexBufferSize();

	size_t vertexStrideSize = model->GetVertexStrideSize();

	D3DGPUSharedAddress vertexBuffer = Gaia::vertexBuffer->AddDataAndGetSharedAddress(
		model->GetVertexData(), vertexBufferSize
	);

	D3DGPUSharedAddress indexBuffer = Gaia::indexBuffer->AddDataAndGetSharedAddress(
		model->GetIndexData(), indexBufferSize
	);

	size_t indexCount = model->GetIndexCount();

	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			D3D12_VERTEX_BUFFER_VIEW{
				0u, static_cast<UINT>(vertexBufferSize), static_cast<UINT>(vertexStrideSize)
			},
			std::move(vertexBuffer),
			D3D12_INDEX_BUFFER_VIEW{
				0u, static_cast<UINT>(indexBufferSize), DXGI_FORMAT_R16_UINT
			},
			std::move(indexBuffer), indexCount,
			std::move(model)
			)
	);
}

void BindInstanceGFX::BindModels(ID3D12GraphicsCommandList* commandList) const noexcept {
	for (auto& model : m_modelsRaw)
		model->Draw(commandList);
}

void BindInstanceGFX::BindPipelineObjects(
	ID3D12GraphicsCommandList* commandList
) const noexcept {
	commandList->SetPipelineState(m_pso->Get());
	commandList->SetGraphicsRootSignature(m_rootSignature->Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void BindInstanceGFX::SetGPUVirtualAddresses() noexcept {
	for (auto& model : m_modelsRaw)
		model->UpdateGPUAddressOffsets();
}

VertexLayout BindInstanceGFX::GetVertexLayout() const noexcept {
	return m_vertexLayout;
}

// Model Raw
BindInstanceGFX::ModelRaw::ModelRaw(std::shared_ptr<IModel>&& model) noexcept
	: m_model(std::move(model)),
	m_indexCount(0u),
	m_indexBufferView{}, m_vertexBufferView{} {}

BindInstanceGFX::ModelRaw::ModelRaw(
	D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView,
	D3DGPUSharedAddress vbvSharedAddress,
	D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
	D3DGPUSharedAddress ibvSharedAddress,
	size_t indexCount,
	std::shared_ptr<IModel>&& model
) noexcept : m_model(std::move(model)),
	m_vertexBufferView(std::move(vertexBufferView), vbvSharedAddress),
	m_indexBufferView(std::move(indexBufferView), ibvSharedAddress),
	m_indexCount(static_cast<UINT>(indexCount)) {}

void BindInstanceGFX::ModelRaw::AddVBV(
	D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView,
	D3DGPUSharedAddress vbvSharedAddress
) noexcept {
	m_vertexBufferView = { std::move(vertexBufferView), vbvSharedAddress };
}

void BindInstanceGFX::ModelRaw::AddIBV(
	D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
	D3DGPUSharedAddress ibvSharedAddress,
	size_t indexCount
) noexcept {
	m_indexBufferView = { std::move(indexBufferView), ibvSharedAddress };
	m_indexCount = static_cast<UINT>(indexCount);
}

void BindInstanceGFX::ModelRaw::UpdateGPUAddressOffsets() {
	m_indexBufferView.SetGPUAddress();
	m_vertexBufferView.SetGPUAddress();
}

void BindInstanceGFX::ModelRaw::Draw(ID3D12GraphicsCommandList* commandList) const noexcept {
	commandList->IASetVertexBuffers(0u, 1u, m_vertexBufferView.GetAddress());
	commandList->IASetIndexBuffer(m_indexBufferView.GetAddress());
	commandList->SetGraphicsRoot32BitConstant(0u, m_model->GetTextureIndex(), 0u);

	TextureOffset texOffset = m_model->GetTextureOffset();
	commandList->SetGraphicsRoot32BitConstants(2u, 2u, &texOffset, 0u);

	DirectX::XMMATRIX modelMat = m_model->GetModelMatrix();
	commandList->SetGraphicsRoot32BitConstants(3u, 16u, &modelMat, 0u);

	commandList->DrawIndexedInstanced(m_indexCount, 1u, 0u, 0u, 0u);
}
