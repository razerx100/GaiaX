#include <GraphicsPipelineMeshShader.hpp>
#include <Shader.hpp>

std::unique_ptr<D3DPipelineObject> GraphicsPipelineMeshShader::_createGraphicsPipelineObject(
	ID3D12Device2* device, const std::wstring& shaderPath, const std::wstring& pixelShader,
	ID3D12RootSignature* graphicsRootSignature
) const noexcept {
	auto ms = std::make_unique<Shader>();
	ms->LoadBinary(shaderPath + L"MeshShader.cso");

	auto ps = std::make_unique<Shader>();
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
	std::uint32_t meshletCount, std::uint32_t meshletOffset
) noexcept {
	ModelDetails modelDetails{
		.meshletOffset = meshletOffset,
		.meshletCount = meshletCount
	};

	m_modelDetails.emplace_back(modelDetails);
}

void GraphicsPipelineMeshShader::DrawModels(
	ID3D12GraphicsCommandList6* graphicsCommandList, const RSLayoutType& graphicsRSLayout
) const noexcept {
	for (const auto& modelDetail : m_modelDetails) {
		static constexpr size_t meshletOffset = static_cast<size_t>(RootSigElement::ModelIndex);

		graphicsCommandList->SetGraphicsRoot32BitConstant(
			graphicsRSLayout[meshletOffset], modelDetail.meshletOffset, 0u
		);

		graphicsCommandList->DispatchMesh(modelDetail.meshletCount, 1u, 1u);
	}
}
