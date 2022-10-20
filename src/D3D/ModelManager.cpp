#include <ModelManager.hpp>
#include <Shader.hpp>
#include <Gaia.hpp>
#include <VertexLayout.hpp>

ModelManager::ModelManager(std::uint32_t bufferCount) noexcept :
	m_renderPipeline{ bufferCount }, m_perFrameBuffers{ bufferCount } {}

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
	m_perFrameBuffers.AddModelInputs(
		std::move(vertices), vertexBufferSize, strideSize, std::move(indices), indexBufferSize
	);
}

void ModelManager::UpdateData(size_t frameIndex) const noexcept {
	m_renderPipeline.UpdateModelData(frameIndex);
	m_perFrameBuffers.UpdateData(frameIndex);
}

void ModelManager::RecordGraphicsCommands(
	ID3D12GraphicsCommandList* commandList, size_t frameIndex
) const noexcept {
	m_renderPipeline.BindGraphicsPipeline(commandList);

	const D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle =
		Gaia::textureStorage->GetTextureDescriptorStart();
	commandList->SetGraphicsRootDescriptorTable(0u, gpuHandle);

	m_perFrameBuffers.BindPerFrameBuffers(commandList, frameIndex, m_graphicsRSLayout);
	m_perFrameBuffers.BindVertexBuffer(commandList);
	m_renderPipeline.DrawModels(commandList, frameIndex);
}

void ModelManager::RecordComputeCommands(
	ID3D12GraphicsCommandList* commandList, size_t frameIndex
) const noexcept {
	m_renderPipeline.BindComputePipeline(commandList);
	// need to bind PerFrameBuffer too
	m_perFrameBuffers.BindPerFrameBuffers(commandList, frameIndex, m_computeRSLayout);
	m_renderPipeline.DispatchCompute(commandList, frameIndex);
}

void ModelManager::ReserveBuffers(ID3D12Device* device) {
	m_renderPipeline.ReserveBuffers(device);
}

void ModelManager::CreateBuffers(ID3D12Device* device) {
	m_renderPipeline.CreateBuffers(device);

	m_perFrameBuffers.SetMemoryAddresses();
}

void ModelManager::RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept {
	m_renderPipeline.RecordResourceUpload(copyList);
}

void ModelManager::ReleaseUploadResource() noexcept {
	m_renderPipeline.ReleaseUploadResource();
}

void ModelManager::InitPipelines(ID3D12Device* device) {
	auto [graphicsPSO, graphicsSignature] = CreateGraphicsPipeline(device);
	auto [computePSO, computeSignature] = CreateComputePipeline(device);

	m_graphicsRSLayout = graphicsSignature->GetElementLayout();
	m_computeRSLayout = computeSignature->GetElementLayout();

	m_renderPipeline.AddGraphicsRootSignature(std::move(graphicsSignature));
	m_renderPipeline.AddGraphicsPipelineObject(std::move(graphicsPSO));
	m_renderPipeline.AddComputeRootSignature(std::move(computeSignature));
	m_renderPipeline.AddComputePipelineObject(std::move(computePSO));

	m_renderPipeline.CreateCommandSignature(device);
}

ModelManager::Pipeline ModelManager::CreateGraphicsPipeline(ID3D12Device* device) const {
	auto signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, D3D12_SHADER_VISIBILITY_PIXEL,
		RootSigElement::Textures, true, 1u
	);
	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_VERTEX,
		RootSigElement::ModelData, false, 0u
	);
	signature->AddConstants(
		1u, D3D12_SHADER_VISIBILITY_VERTEX, RootSigElement::TextureIndex, 0u
	);
	signature->AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_VERTEX, RootSigElement::Camera, 1u
	);

	signature->CompileSignature();
	signature->CreateSignature(device);

	auto vs = std::make_unique<Shader>();
	vs->LoadBinary(m_shaderPath + L"VertexShader.cso");

	auto ps = std::make_unique<Shader>();
	ps->LoadBinary(m_shaderPath + L"PixelShader.cso");

	VertexLayout vertexLayout{};
	vertexLayout.AddInputElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 12u);
	vertexLayout.AddInputElement("UV", DXGI_FORMAT_R32G32_FLOAT, 8u);

	auto pso = std::make_unique<D3DPipelineObject>();
	pso->CreateGFXPipelineState(
		device, vertexLayout.GetLayoutDesc(), signature->Get(), vs->GetByteCode(),
		ps->GetByteCode()
	);

	return { std::move(pso), std::move(signature) };
}

ModelManager::Pipeline ModelManager::CreateComputePipeline(ID3D12Device* device) const {
	auto signature = std::make_unique<RootSignatureDynamic>();
	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::ModelData, false, 0u
	);
	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::IndirectArgsSRV,false, 1u
	);
	signature->AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_ALL, RootSigElement::Camera, 0u
	);

	signature->CompileSignature();
	signature->CreateSignature(device);

	auto cs = std::make_unique<Shader>();
	cs->LoadBinary(m_shaderPath + L"ComputeShader.cso");

	auto pso = std::make_unique<D3DPipelineObject>();
	pso->CreateComputePipelineState(device, signature->Get(), cs->GetByteCode());

	return { std::move(pso), std::move(signature) };
}

ID3D12Resource* ModelManager::GetCommandBuffer() const noexcept {
	return m_renderPipeline.GetCommandBuffer();
}
