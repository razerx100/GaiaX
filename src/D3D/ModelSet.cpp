#include <ModelSet.hpp>

// Model Instance
ModelInstance::ModelInstance(std::shared_ptr<IModel>&& model) noexcept
	: m_model{ std::move(model) } {}

void ModelInstance::Draw(ID3D12GraphicsCommandList* commandList) const noexcept {
	commandList->SetGraphicsRoot32BitConstant(0u, m_model->GetTextureIndex(), 0u);

	const UVInfo uvInfo = m_model->GetUVInfo();
	commandList->SetGraphicsRoot32BitConstants(2u, 4u, &uvInfo, 0u);

	const DirectX::XMMATRIX modelMat = m_model->GetModelMatrix();
	commandList->SetGraphicsRoot32BitConstants(3u, 16u, &modelMat, 0u);

	const UINT indexCount = m_model->GetIndexCount();
	commandList->DrawIndexedInstanced(indexCount, 1u, 0u, 0u, 0u);
}

// Model Set Vertex
void ModelSetVertex::AddInstance(std::shared_ptr<IModel>&& model) noexcept {
	m_modelInstances.emplace_back(std::move(model));
}

void ModelSetVertex::DrawInstances(ID3D12GraphicsCommandList* commandList) const noexcept {
	for (const auto& instance : m_modelInstances)
		instance.Draw(commandList);
}

// Model Set Per Vertex
ModelSetPerVertex::ModelSetPerVertex(
	D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView, D3DGPUSharedAddress vbvSharedAddress,
	D3D12_INDEX_BUFFER_VIEW&& indexBufferView, D3DGPUSharedAddress ibvSharedAddress
) noexcept : m_vertexBufferView{ std::move(vertexBufferView), vbvSharedAddress },
	m_indexBufferView{ std::move(indexBufferView), ibvSharedAddress } {}

void ModelSetPerVertex::BindInputs(ID3D12GraphicsCommandList* commandList) const noexcept {
	commandList->IASetVertexBuffers(0u, 1u, m_vertexBufferView.GetAddress());
	commandList->IASetIndexBuffer(m_indexBufferView.GetAddress());
}

void ModelSetPerVertex::UpdateGPUAddressOffsets() noexcept {
	m_vertexBufferView.SetGPUAddress();
	m_indexBufferView.SetGPUAddress();
}

// Model Set Global Vertex
ModelSetGVertex::ModelSetGVertex(
	D3D12_INDEX_BUFFER_VIEW&& indexBufferView, D3DGPUSharedAddress ibvSharedAddress
) noexcept : m_indexBufferView{ std::move(indexBufferView), ibvSharedAddress } {}

void ModelSetGVertex::UpdateGPUAddressOffsets() noexcept {
	m_indexBufferView.SetGPUAddress();
}

void ModelSetGVertex::BindInputs(ID3D12GraphicsCommandList* commandList) const noexcept {
	commandList->IASetIndexBuffer(m_indexBufferView.GetAddress());
}
