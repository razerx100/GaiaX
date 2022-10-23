#include <BufferManager.hpp>
#include <ranges>
#include <algorithm>
#include <Gaia.hpp>

#include <CameraManager.hpp>

BufferManager::BufferManager(std::uint32_t frameCount)
	: m_cameraBuffer{}, m_gVertexBufferView{}, m_gIndexBufferView{},
	m_modelBuffers{ ResourceType::cpuWrite }, m_frameCount{ frameCount } {}

void BufferManager::ReserveBuffers(ID3D12Device* device) noexcept {
	size_t cameraBufferSize = sizeof(DirectX::XMMATRIX) * 2u;
	constexpr size_t cameraBufferAlignment = 256u;
	size_t cameraOffset = Gaia::Resources::cpuWriteBuffer->ReserveSpaceAndGetOffset(
		cameraBufferSize, m_frameCount, cameraBufferAlignment
	);

	m_cameraBuffer.SetAddressesStart(cameraOffset, cameraBufferSize, cameraBufferAlignment);

	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const size_t modelBufferDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);

	m_modelBuffers.SetDescriptorOffset(modelBufferDescriptorOffset, descriptorSize);
	m_modelBuffers.SetBufferInfo(
		device, static_cast<UINT>(sizeof(ModelConstantBuffer)),
		static_cast<UINT>(std::size(m_opaqueModels)), m_frameCount
	);
}

void BufferManager::CreateBuffers(ID3D12Device* device) {
	const D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart =
		Gaia::descriptorTable->GetUploadDescriptorStart();

	const D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart =
		Gaia::descriptorTable->GetGPUDescriptorStart();

	m_modelBuffers.CreateDescriptorView(
		device, uploadDescriptorStart, gpuDescriptorStart, D3D12_RESOURCE_STATE_GENERIC_READ
	);
}

void BufferManager::SetMemoryAddresses() noexcept {
	std::uint8_t* cpuOffset = Gaia::Resources::cpuWriteBuffer->GetCPUStartAddress();
	D3D12_GPU_VIRTUAL_ADDRESS gpuOffset = Gaia::Resources::cpuWriteBuffer->GetGPUStartAddress();

	m_cameraBuffer.UpdateCPUAddressStart(cpuOffset);
	m_cameraBuffer.UpdateGPUAddressStart(gpuOffset);

	const D3D12_GPU_VIRTUAL_ADDRESS vertexGpuStart =
		Gaia::Resources::vertexBuffer->GetGPUStartAddress();

	m_gVertexBufferView.OffsetGPUAddress(vertexGpuStart);
	m_gIndexBufferView.OffsetGPUAddress(vertexGpuStart);
}

void BufferManager::Update(size_t frameIndex) const noexcept {
	std::uint8_t* cameraCpuHandle = m_cameraBuffer.GetCPUAddressStart(frameIndex);

	Gaia::cameraManager->CopyData(cameraCpuHandle);

	UpdateModelData(frameIndex);
}

void BufferManager::BindBuffersToGraphics(
	ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex
) const noexcept {
	BindBuffers<
		&ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView,
		&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
	>(graphicsCmdList, frameIndex, m_graphicsRSLayout);
}

void BufferManager::BindBuffersToCompute(
	ID3D12GraphicsCommandList* computeCmdList, size_t frameIndex
) const noexcept {
	BindBuffers<
		&ID3D12GraphicsCommandList::SetComputeRootConstantBufferView,
		&ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
	>(computeCmdList, frameIndex, m_computeRSLayout);
}

void BufferManager::BindVertexBuffer(
	ID3D12GraphicsCommandList* graphicsCmdList
) const noexcept {
	graphicsCmdList->IASetVertexBuffers(0u, 1u, m_gVertexBufferView.GetAddress());
	graphicsCmdList->IASetIndexBuffer(m_gIndexBufferView.GetAddress());
}

void BufferManager::AddModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	const size_t vertexOffset = Gaia::Resources::vertexBuffer->ReserveSpaceAndGetOffset(
		vertexBufferSize
	);
	const size_t indexOffset = Gaia::Resources::vertexBuffer->ReserveSpaceAndGetOffset(
		indexBufferSize
	);

	Gaia::Resources::vertexUploadContainer->AddMemory(
		std::move(vertices), vertexBufferSize, vertexOffset
	);
	Gaia::Resources::vertexUploadContainer->AddMemory(
		std::move(indices), indexBufferSize, indexOffset
	);

	m_gVertexBufferView.AddBufferView(
		D3D12_VERTEX_BUFFER_VIEW{
			static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(vertexOffset),
			static_cast<UINT>(vertexBufferSize), static_cast<UINT>(strideSize)
		}
	);

	m_gIndexBufferView.AddBufferView(
		D3D12_INDEX_BUFFER_VIEW{
			static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(indexOffset),
			static_cast<UINT>(indexBufferSize), DXGI_FORMAT_R32_UINT
		}
	);
}

void BufferManager::SetComputeRootSignatureLayout(std::vector<UINT> rsLayout) noexcept {
	m_computeRSLayout = std::move(rsLayout);
}

void BufferManager::SetGraphicsRootSignatureLayout(std::vector<UINT> rsLayout) noexcept {
	m_graphicsRSLayout = std::move(rsLayout);
}

void BufferManager::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

void BufferManager::UpdateModelData(size_t frameIndex) const noexcept {
	size_t offset = 0u;
	static constexpr size_t bufferStride = sizeof(ModelConstantBuffer);

	for (auto& model : m_opaqueModels) {
		ModelConstantBuffer modelBuffer{};
		modelBuffer.textureIndex = model->GetTextureIndex();
		modelBuffer.uvInfo = model->GetUVInfo();
		modelBuffer.modelMatrix = model->GetModelMatrix();

		memcpy(
			m_modelBuffers.GetCPUWPointer(frameIndex) + offset, &modelBuffer, bufferStride
		);

		offset += bufferStride;
	}
}
