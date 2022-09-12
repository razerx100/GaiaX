#include <ModelManager.hpp>
#include <Shader.hpp>
#include <Gaia.hpp>
#include <VertexLayout.hpp>

ModelManager::ModelManager(std::uint32_t bufferCount) noexcept :
	m_renderPipeline{ bufferCount }, m_pPerFrameBuffers{ bufferCount } {}

void ModelManager::SetShaderPath(std::wstring path) noexcept {
	m_shaderPath = std::move(path);
}

void ModelManager::AddModels(std::vector<std::shared_ptr<IModel>>&& models) {
	m_renderPipeline.AddOpaqueModels(std::move(models));
}

void ModelManager::AddModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	m_pPerFrameBuffers.AddModelInputs(
		std::move(vertices), vertexBufferSize, strideSize,
		std::move(indices), indexBufferSize
	);
}

void ModelManager::BindCommands(
	ID3D12GraphicsCommandList* commandList, size_t frameIndex
) const noexcept {
	m_renderPipeline.BindGraphicsPipeline(commandList);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = Gaia::textureStorage->GetTextureDescriptorStart();
	commandList->SetGraphicsRootDescriptorTable(0u, gpuHandle);

	m_pPerFrameBuffers.BindPerFrameBuffers(commandList, frameIndex);
	m_renderPipeline.UpdateModelData(frameIndex);
	m_renderPipeline.DrawModels(commandList, frameIndex);
}

void ModelManager::ReserveBuffers(ID3D12Device* device) {
	m_renderPipeline.ReserveCommandBuffers(device);
}

void ModelManager::CreateBuffers(ID3D12Device* device) {
	m_renderPipeline.CreateCommandBuffers(device);

	m_pPerFrameBuffers.SetMemoryAddresses();
}

void ModelManager::RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept {
	m_renderPipeline.RecordResourceUpload(copyList);
}

void ModelManager::ReleaseUploadResource() noexcept {
	m_renderPipeline.ReleaseUploadResource();
}

void ModelManager::InitPipelines(ID3D12Device* device) {
	auto [pso, signature] = CreatePipeline(device);

	m_renderPipeline.AddGraphicsRootSignature(std::move(signature));
	m_renderPipeline.AddGraphicsPipelineObject(std::move(pso));

	m_renderPipeline.CreateCommandSignature(device);
}

ModelManager::Pipeline ModelManager::CreatePipeline(ID3D12Device* device) const {
	std::unique_ptr<RootSignatureDynamic> signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, D3D12_SHADER_VISIBILITY_PIXEL, true,
		1u
	);
	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_VERTEX, false, 0u
	);
	signature->AddConstants(1u, D3D12_SHADER_VISIBILITY_VERTEX, 0u);
	signature->AddConstantBufferView(D3D12_SHADER_VISIBILITY_VERTEX, 1u);

	signature->CompileSignature();
	signature->CreateSignature(device);

	std::unique_ptr<Shader> vs = std::make_unique<Shader>();
	vs->LoadBinary(m_shaderPath + L"VertexShader.cso");

	std::unique_ptr<Shader> ps = std::make_unique<Shader>();
	ps->LoadBinary(m_shaderPath + L"PixelShader.cso");

	VertexLayout vertexLayout{};
	vertexLayout.AddInputElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 12u);
	vertexLayout.AddInputElement("UV", DXGI_FORMAT_R32G32_FLOAT, 8u);

	std::unique_ptr<PipelineObjectGFX> pso = std::make_unique<PipelineObjectGFX>();
	pso->CreatePipelineState(
		device,
		vertexLayout.GetLayoutDesc(),
		signature->Get(),
		vs->GetByteCode(),
		ps->GetByteCode()
	);

	return {
		std::move(pso),
		std::move(signature)
	};
}
