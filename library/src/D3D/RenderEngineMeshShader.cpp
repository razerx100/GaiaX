#include <ranges>
#include <algorithm>
#include <RenderEngineMeshShader.hpp>
#include <Gaia.hpp>

RenderEngineMeshDraw::RenderEngineMeshDraw(ID3D12Device* device) noexcept
	: RenderEngineBase{ device }, m_meshletBuffer{ DescriptorType::SRV } {}

void RenderEngineMeshDraw::ExecuteRenderStage(size_t frameIndex) {
	ID3D12GraphicsCommandList6* graphicsCommandList = Gaia::graphicsCmdList->GetCommandList6();

	ExecutePreGraphicsStage(graphicsCommandList, frameIndex);
	RecordDrawCommands(graphicsCommandList, frameIndex);
}

void RenderEngineMeshDraw::BindGraphicsBuffers(
	ID3D12GraphicsCommandList* graphicsCommandList, size_t frameIndex
) {
	BindCommonGraphicsBuffers(graphicsCommandList, frameIndex);
	m_vertexManager.BindVertexBuffers(graphicsCommandList, m_graphicsRSLayout);

	static constexpr auto meshletsIndex = static_cast<size_t>(RootSigElement::Meshlets);
	graphicsCommandList->SetGraphicsRootDescriptorTable(
		m_graphicsRSLayout[meshletsIndex], m_meshletBuffer.GetFirstGPUDescriptorHandle()
	);
}

void RenderEngineMeshDraw::RecordDrawCommands(
	ID3D12GraphicsCommandList6* graphicsCommandList, size_t frameIndex
) {
	ID3D12RootSignature* graphicsRS = m_graphicsRS->Get();

	// One Pipeline needs to be bound before Descriptors can be bound.
	m_graphicsPipeline0->BindGraphicsPipeline(graphicsCommandList, graphicsRS);
	BindGraphicsBuffers(graphicsCommandList, frameIndex);

	m_graphicsPipeline0->DrawModels(graphicsCommandList, m_graphicsRSLayout);

	for (auto& graphicsPipeline : m_graphicsPipelines) {
		graphicsPipeline->BindGraphicsPipeline(graphicsCommandList, graphicsRS);
		graphicsPipeline->DrawModels(graphicsCommandList, m_graphicsRSLayout);
	}
}

void RenderEngineMeshDraw::UpdateModelBuffers(size_t frameIndex) const noexcept {
	Gaia::bufferManager->Update<true>(frameIndex);
}

/*void RenderEngineMeshDraw::AddMeshletModelSet(
	std::vector<MeshletModel>& meshletModels, const std::wstring& pixelShader
) noexcept {
	auto graphicsPipeline = std::make_unique<GraphicsPipelineMeshShader>();

	graphicsPipeline->ConfigureGraphicsPipelineObject(pixelShader);

	static std::uint32_t modelCount = 0u;

	for (size_t index = 0u; index < std::size(meshletModels); ++index) {
		std::vector<Meshlet>&& meshlets = std::move(meshletModels[index].meshlets);

		graphicsPipeline->AddModelDetails(
			static_cast<std::uint32_t>(std::size(meshlets)),
			static_cast<std::uint32_t>(std::size(m_meshlets)), modelCount
		);

		++modelCount;

		std::ranges::move(meshlets, std::back_inserter(m_meshlets));
	}

	if (!m_graphicsPipeline0)
		m_graphicsPipeline0 = std::move(graphicsPipeline);
	else
		m_graphicsPipelines.emplace_back(std::move(graphicsPipeline));
}*/

void RenderEngineMeshDraw::AddGVerticesAndPrimIndices(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
	std::vector<std::uint32_t>&& gPrimIndices
) noexcept {
	m_vertexManager.AddGVerticesAndPrimIndices(
		std::move(gVertices), std::move(gVerticesIndices), std::move(gPrimIndices)
	);
}

void RenderEngineMeshDraw::CreateBuffers(ID3D12Device* device) {
	m_vertexManager.CreateBuffers(device);

	CreateUploadDescView(device, m_meshletBuffer, m_meshlets);
}

void RenderEngineMeshDraw::RecordResourceUploads(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_vertexManager.RecordResourceUpload(copyList);
	m_meshletBuffer.RecordResourceUpload(copyList);
}

void RenderEngineMeshDraw::ReleaseUploadResources() noexcept {
	m_vertexManager.ReleaseUploadResource();
	m_meshletBuffer.ReleaseUploadResource();

	m_meshlets = std::vector<Meshlet>{};
}

void RenderEngineMeshDraw::ReserveBuffersDerived(ID3D12Device* device) {
	m_vertexManager.ReserveBuffers(device);

	const size_t meshletDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();

	SetDescBufferInfo(device, meshletDescriptorOffset, m_meshlets, m_meshletBuffer);
}

void RenderEngineMeshDraw::ConstructPipelines() {
	ID3D12Device2* device = Gaia::device->GetDevice();

	ConstructGraphicsRootSignature(device);
	CreateGraphicsPipelines(device, m_graphicsPipeline0, m_graphicsPipelines);
}

std::unique_ptr<RootSignatureDynamic> RenderEngineMeshDraw::CreateGraphicsRootSignature(
	ID3D12Device* device
) const noexcept {
	auto signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, D3D12_SHADER_VISIBILITY_PIXEL,
		RootSigElement::Textures, true, 0u, 1u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_MESH,
		RootSigElement::ModelData, false, 0u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_PIXEL,
		RootSigElement::MaterialData, false, 1u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_PIXEL,
		RootSigElement::LightData, false, 2u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_MESH,
		RootSigElement::VertexData, false, 3u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_MESH,
		RootSigElement::VertexIndices, false, 4u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_MESH,
		RootSigElement::PrimIndices, false, 5u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_MESH,
		RootSigElement::Meshlets, false, 6u
	).AddConstants(
		2u, D3D12_SHADER_VISIBILITY_MESH, RootSigElement::ModelInfo, 0u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_MESH, RootSigElement::Camera, 1u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_PIXEL, RootSigElement::PixelData, 2u
	).CompileSignature().CreateSignature(device);

	return signature;
}
