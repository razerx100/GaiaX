#include <BindInstanceGFX.hpp>
#include <InstanceManager.hpp>
#include <VenusInstance.hpp>

BindInstanceGFX::BindInstanceGFX(
bool textureAvailable
) noexcept : m_textureAvailable(textureAvailable) {}

BindInstanceGFX::BindInstanceGFX(
	bool textureAvailable,
	std::unique_ptr<IPipelineObject> pso,
	std::shared_ptr<IRootSignature> signature
) noexcept
	: m_pso(std::move(pso)), m_rootSignature(signature), m_textureAvailable(textureAvailable) {}

void BindInstanceGFX::AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept {
	m_pso = std::move(pso);
}

void BindInstanceGFX::AddRootSignature(std::shared_ptr<IRootSignature> signature) noexcept {
	m_rootSignature = signature;
}

void BindInstanceGFX::AddModel(
	ID3D12Device* device, const IModel* const modelRef
) noexcept {
	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(modelRef)
	);
}

void BindInstanceGFX::BindCommands(ID3D12GraphicsCommandList* commandList) noexcept {
	commandList->SetPipelineState(m_pso->Get());
	commandList->SetGraphicsRootSignature(m_rootSignature->Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& model : m_modelsRaw)
		model->Draw(commandList);
}

void BindInstanceGFX::CopyData(
	ID3D12Device* device
) {
	std::atomic_uint32_t works = 2u;

	size_t vertexBufferSize = 0u;
	size_t indexBufferSize = 0u;

	IResourceBuffer* vertexBufferRef = VertexBufferInst::GetRef();
	IResourceBuffer* indexBufferRef = IndexBufferInst::GetRef();

	if (m_textureAvailable) {
		for (auto& modelRaw : m_modelsRaw) {
			const IModel* const modelRef = modelRaw->GetModelRef();

			const std::vector<std::uint16_t>& indicesRef = modelRef->GetIndices();
			const std::vector<Ceres::Float32_3>& verticesRef = modelRef->GetVertices();

			size_t currentIndicesSize =
				indicesRef.size() * sizeof(decltype(indicesRef[0]));

			size_t vertexStrideSize = sizeof(decltype(verticesRef[0]));
			size_t currentVerticesSize =
				verticesRef.size() * vertexStrideSize;

			modelRaw->AddIBV(
				D3D12_INDEX_BUFFER_VIEW{
					indexBufferSize,
					static_cast<UINT>(currentIndicesSize),
					DXGI_FORMAT_R16_UINT
				},
				static_cast<std::uint64_t>(indicesRef.size())
			);

			modelRaw->AddVBV(
				D3D12_VERTEX_BUFFER_VIEW{
					vertexBufferSize,
					static_cast<UINT>(currentVerticesSize),
					static_cast<UINT>(vertexStrideSize)
				}
			);

			indexBufferSize += currentIndicesSize;
			vertexBufferSize += currentVerticesSize;
		}

		vertexBufferRef->CreateBuffer(device, vertexBufferSize);
		indexBufferRef->CreateBuffer(device, indexBufferSize);

		for (auto& modelRaw : m_modelsRaw) {
			modelRaw->UpdateIBVGPUOffset(indexBufferRef->GetGPUHandle());
			modelRaw->UpdateVBVGPUOffset(vertexBufferRef->GetGPUHandle());
		}

		GetVenusInstance()->SubmitWork([&] {
			for (auto& modelRaw : m_modelsRaw) {
				const std::vector<Ceres::Float32_3>& verticesRef =
					modelRaw->GetModelRef()->GetVertices();

				vertexBufferRef->CopyData(
					verticesRef.data(),
					verticesRef.size() * sizeof(decltype(verticesRef[0]))
				);
			}

			--works;
			}
		);

		GetVenusInstance()->SubmitWork([&] {
			for (auto& modelRaw : m_modelsRaw) {
				const std::vector<std::uint16_t>& indicesRef =
					modelRaw->GetModelRef()->GetIndices();

				indexBufferRef->CopyData(
					indicesRef.data(),
					indicesRef.size() * sizeof(decltype(indicesRef[0]))
				);
			}

			--works;
			}
		);
	}
	else {
		for (auto& modelRaw : m_modelsRaw) {
			const IModel* const modelRef = modelRaw->GetModelRef();

			const std::vector<std::uint16_t>& indicesRef = modelRef->GetIndices();
			const std::vector<Ceres::Float32_3>& verticesRef = modelRef->GetVertices();

			size_t currentIndicesSize =
				indicesRef.size() * sizeof(decltype(indicesRef[0]));

			size_t vertexStrideSize =
				sizeof(decltype(verticesRef[0])) + sizeof(Ceres::VectorF32);
			size_t currentVerticesSize =
				verticesRef.size() * vertexStrideSize;

			modelRaw->AddIBV(
				D3D12_INDEX_BUFFER_VIEW{
					indexBufferSize,
					static_cast<UINT>(currentIndicesSize),
					DXGI_FORMAT_R16_UINT
				},
				static_cast<std::uint64_t>(indicesRef.size())
			);

			modelRaw->AddVBV(
				D3D12_VERTEX_BUFFER_VIEW{
					vertexBufferSize,
					static_cast<UINT>(currentVerticesSize),
					static_cast<UINT>(vertexStrideSize)
				}
			);

			indexBufferSize += currentIndicesSize;
			vertexBufferSize += currentVerticesSize;
		}

		vertexBufferRef->CreateBuffer(device, vertexBufferSize);
		indexBufferRef->CreateBuffer(device, indexBufferSize);

		for (auto& modelRaw : m_modelsRaw) {
			modelRaw->UpdateIBVGPUOffset(indexBufferRef->GetGPUHandle());
			modelRaw->UpdateVBVGPUOffset(vertexBufferRef->GetGPUHandle());
		}

		GetVenusInstance()->SubmitWork([&] {
			for (auto& modelRaw : m_modelsRaw) {
				const std::vector<Ceres::Float32_3>& verticesRef =
					modelRaw->GetModelRef()->GetVertices();

				const Ceres::VectorF32 solidColor = modelRaw->GetModelRef()->GetSolidColor();

				for (auto& vertex : verticesRef) {
					vertexBufferRef->CopyData(&vertex, sizeof(Ceres::Float32_3));
					vertexBufferRef->CopyData(&solidColor, sizeof(Ceres::VectorF32));
				}
			}

			--works;
			}
		);

		GetVenusInstance()->SubmitWork([&] {
			for (auto& modelRaw : m_modelsRaw) {
				const std::vector<std::uint16_t>& indicesRef =
					modelRaw->GetModelRef()->GetIndices();

				indexBufferRef->CopyData(
					indicesRef.data(),
					indicesRef.size() * sizeof(decltype(indicesRef[0]))
				);
			}

			--works;
			}
		);
	}

	while (works != 0u);
}

void BindInstanceGFX::RecordUploadBuffers(ID3D12GraphicsCommandList* copyList) {
	VertexBufferInst::GetRef()->RecordUpload(copyList, BufferType::Vertex);
	IndexBufferInst::GetRef()->RecordUpload(copyList, BufferType::Index);
}

// Model Raw
BindInstanceGFX::ModelRaw::ModelRaw(const IModel* const modelRef) noexcept
	:
	m_vertexBufferView{},
	m_indexBufferView{},
	m_indicesCount(0u),
	m_modelRef(modelRef) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView,
	const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
	std::uint64_t indicesCount,
	const IModel* const modelRef
) noexcept
	:
	m_vertexBufferView(vertexBufferView),
	m_indexBufferView(indexBufferView),
	m_indicesCount(indicesCount),
	m_modelRef(modelRef) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView,
	D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
	std::uint64_t indicesCount,
	const IModel* const modelRef
) noexcept
	:
	m_vertexBufferView(std::move(vertexBufferView)),
	m_indexBufferView(std::move(indexBufferView)),
	m_indicesCount(indicesCount),
	m_modelRef(modelRef) {}

void BindInstanceGFX::ModelRaw::AddVBV(const D3D12_VERTEX_BUFFER_VIEW& vertexBufferView) noexcept {
	m_vertexBufferView = vertexBufferView;
}

void BindInstanceGFX::ModelRaw::AddVBV(D3D12_VERTEX_BUFFER_VIEW&& vertexBufferView) noexcept {
	m_vertexBufferView = std::move(vertexBufferView);
}

void BindInstanceGFX::ModelRaw::AddIBV(
	const D3D12_INDEX_BUFFER_VIEW& indexBufferView,
	std::uint64_t indicesCount
) noexcept {
	m_indexBufferView = indexBufferView;
	m_indicesCount = indicesCount;
}

void BindInstanceGFX::ModelRaw::AddIBV(
	D3D12_INDEX_BUFFER_VIEW&& indexBufferView,
	std::uint64_t indicesCount
) noexcept {
	m_indexBufferView = std::move(indexBufferView);
	m_indicesCount = indicesCount;
}

void BindInstanceGFX::ModelRaw::Draw(ID3D12GraphicsCommandList* commandList) noexcept {
	commandList->IASetVertexBuffers(0u, 1u, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);

	commandList->DrawIndexedInstanced(m_indicesCount, 1u, 0u, 0u, 0u);
}

const IModel* const BindInstanceGFX::ModelRaw::GetModelRef() const noexcept {
	return m_modelRef;
}

void BindInstanceGFX::ModelRaw::UpdateVBVGPUOffset(std::uint64_t offset) noexcept {
	m_vertexBufferView.BufferLocation += offset;
}

void BindInstanceGFX::ModelRaw::UpdateIBVGPUOffset(std::uint64_t offset) noexcept {
	m_indexBufferView.BufferLocation += offset;
}
