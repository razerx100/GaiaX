#include <BindInstanceGFX.hpp>
#include <Gaia.hpp>
#include <DirectXMath.h>

// Bind Instance Base
BindInstanceGFX::BindInstanceGFX(
	std::unique_ptr<PipelineObjectGFX> pso, std::unique_ptr<RootSignatureDynamic> signature
) noexcept : m_pso(std::move(pso)), m_rootSignature(std::move(signature)) {}

void BindInstanceGFX::AddPSO(std::unique_ptr<PipelineObjectGFX> pso) noexcept {
	m_pso = std::move(pso);
}

void BindInstanceGFX::AddRootSignature(
	std::unique_ptr<RootSignatureDynamic> signature
) noexcept {
	m_rootSignature = std::move(signature);
}

void BindInstanceGFX::BindPipelineObjects(ID3D12GraphicsCommandList* commandList) const noexcept {
	commandList->SetPipelineState(m_pso->Get());
	commandList->SetGraphicsRootSignature(m_rootSignature->Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// Bind Instance Per Model Vertex
void BindInstancePerVertex::AddModels(
	std::vector<std::shared_ptr<IModel>>&& models, std::unique_ptr<IModelInputs> modelInputs
) noexcept {
	size_t indexBufferSize = modelInputs->GetIndexBufferSize();
	size_t vertexBufferSize = modelInputs->GetVertexBufferSize();

	size_t vertexStrideSize = modelInputs->GetVertexStrideSize();

	D3DGPUSharedAddress vertexBuffer = Gaia::vertexBuffer->AddDataAndGetSharedAddress(
		modelInputs->GetVertexData(), vertexBufferSize
	);

	D3DGPUSharedAddress indexBuffer = Gaia::indexBuffer->AddDataAndGetSharedAddress(
		modelInputs->GetIndexData(), indexBufferSize
	);

	auto modelSet = std::make_unique<ModelSetPerVertex>(
		D3D12_VERTEX_BUFFER_VIEW{
			0u, static_cast<UINT>(vertexBufferSize), static_cast<UINT>(vertexStrideSize)
		},
		std::move(vertexBuffer),
		D3D12_INDEX_BUFFER_VIEW{
			0u, static_cast<UINT>(indexBufferSize), DXGI_FORMAT_R16_UINT
		},
		std::move(indexBuffer)
		);

	for (auto& model : models)
		modelSet->AddInstance(std::move(model));

	m_models.emplace_back(std::move(modelSet));
}

void BindInstancePerVertex::DrawModels(ID3D12GraphicsCommandList* commandList) const noexcept {
	for (const auto& model : m_models) {
		model->BindInputs(commandList);
		model->DrawInstances(commandList);
	}
}

void BindInstancePerVertex::SetGPUVirtualAddresses() noexcept {
	for (auto& model : m_models)
		model->UpdateGPUAddressOffsets();
}

// Bind Instance Per Model Vertex
// This won't work. It's a placeholder for now
void BindInstanceGVertex::AddModels(
	std::vector<std::shared_ptr<IModel>>&& models, std::unique_ptr<IModelInputs> modelInputs
) noexcept {
	size_t indexBufferSize = modelInputs->GetIndexBufferSize();
	size_t vertexBufferSize = modelInputs->GetVertexBufferSize();

	size_t vertexStrideSize = modelInputs->GetVertexStrideSize();

	D3DGPUSharedAddress vertexBuffer = Gaia::vertexBuffer->AddDataAndGetSharedAddress(
		modelInputs->GetVertexData(), vertexBufferSize
	);

	D3DGPUSharedAddress indexBuffer = Gaia::indexBuffer->AddDataAndGetSharedAddress(
		modelInputs->GetIndexData(), indexBufferSize
	);

	m_vertexBufferView = {
		D3D12_VERTEX_BUFFER_VIEW{
			0u, static_cast<UINT>(vertexBufferSize), static_cast<UINT>(vertexStrideSize)
		},
		std::move(vertexBuffer)
	};

	auto modelSet = std::make_unique<ModelSetGVertex>(
		D3D12_INDEX_BUFFER_VIEW{
			0u, static_cast<UINT>(indexBufferSize), DXGI_FORMAT_R16_UINT
		},
		std::move(indexBuffer)
		);

	for (auto& model : models)
		modelSet->AddInstance(std::move(model));

	m_models.emplace_back(std::move(modelSet));
}

void BindInstanceGVertex::DrawModels(ID3D12GraphicsCommandList* commandList) const noexcept {
	commandList->IASetVertexBuffers(0u, 1u, m_vertexBufferView.GetAddress());

	for (const auto& model : m_models) {
		model->BindInputs(commandList);
		model->DrawInstances(commandList);
	}
}

void BindInstanceGVertex::SetGPUVirtualAddresses() noexcept {
	m_vertexBufferView.SetGPUAddress();

	for (auto& model : m_models)
		model->UpdateGPUAddressOffsets();
}

