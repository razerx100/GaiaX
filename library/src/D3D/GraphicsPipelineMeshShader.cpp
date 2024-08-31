#include <GraphicsPipelineMeshShader.hpp>
#include <D3DShader.hpp>

std::unique_ptr<D3DPipelineObject> GraphicsPipelineMeshShader::_createGraphicsPipelineObject(
	ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
	ID3D12RootSignature* graphicsRootSignature
) const noexcept {
	auto ms = std::make_unique<D3DShader>();
	ms->LoadBinary(shaderPath + L"MeshShader.cso");

	auto ps = std::make_unique<D3DShader>();
	ps->LoadBinary(shaderPath + pixelShader);

	auto pso = std::make_unique<D3DPipelineObject>();
	pso->CreateGFXPipelineStateMesh(
		device, graphicsRootSignature, ms->GetByteCode(), ps->GetByteCode()
	);

	return pso;
}

void GraphicsPipelineMeshShader::ConfigureGraphicsPipelineObject(
	const std::wstring& pixelShader
) noexcept {
	m_pixelShader = pixelShader;
}

void GraphicsPipelineMeshShader::AddModelDetails(
	std::uint32_t meshletCount, std::uint32_t meshletOffset, std::uint32_t modelIndex
) noexcept {
	ModelDetails modelDetails{
		.modelIndex = modelIndex,
		.meshletOffset = meshletOffset,
		.meshletCount = meshletCount
	};

	m_modelDetails.emplace_back(modelDetails);
}

void GraphicsPipelineMeshShader::DrawModels(
	ID3D12GraphicsCommandList6* graphicsCommandList//, const RSLayoutType& graphicsRSLayout
) const noexcept {
	for (const auto& modelDetail : m_modelDetails) {
		/*
		static constexpr size_t modelInfoIndex = static_cast<size_t>(RootSigElement::ModelInfo);

		graphicsCommandList->SetGraphicsRoot32BitConstants(
			graphicsRSLayout[modelInfoIndex], 2u, &modelDetail.modelIndex, 0u
		);
		*/

		graphicsCommandList->DispatchMesh(modelDetail.meshletCount, 1u, 1u);
	}
}
