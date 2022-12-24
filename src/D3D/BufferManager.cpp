#include <BufferManager.hpp>
#include <ranges>
#include <algorithm>
#include <Gaia.hpp>

#include <CameraManager.hpp>

BufferManager::BufferManager(std::uint32_t frameCount)
	: m_cameraBuffer{}, m_modelBuffers{ ResourceType::cpuWrite, DescriptorType::SRV },
	m_frameCount{ frameCount } {}

void BufferManager::ReserveBuffers(ID3D12Device* device) noexcept {
	size_t cameraBufferSize = sizeof(DirectX::XMMATRIX) * 2u;
	constexpr size_t cameraBufferAlignment = 256u;
	size_t cameraOffset =
		Gaia::Resources::cpuWriteBuffer->ReserveSpaceSuballocatedAndGetOffset(
			cameraBufferSize, m_frameCount, cameraBufferAlignment
		);

	m_cameraBuffer.SetAddressesStart(cameraOffset, cameraBufferSize, cameraBufferAlignment);

	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const size_t modelBufferDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);

	m_modelBuffers.SetDescriptorOffset(modelBufferDescriptorOffset, descriptorSize);
	m_modelBuffers.SetBufferInfo(
		device, static_cast<UINT>(sizeof(ModelBuffer)),
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

	SetMemoryAddresses();
}

void BufferManager::SetMemoryAddresses() noexcept {
	std::uint8_t* cpuOffset = Gaia::Resources::cpuWriteBuffer->GetCPUStartAddress();
	D3D12_GPU_VIRTUAL_ADDRESS gpuOffset = Gaia::Resources::cpuWriteBuffer->GetGPUStartAddress();

	m_cameraBuffer.UpdateCPUAddressStart(cpuOffset);
	m_cameraBuffer.UpdateGPUAddressStart(gpuOffset);
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

void BufferManager::SetComputeRootSignatureLayout(RSLayoutType rsLayout) noexcept {
	m_computeRSLayout = std::move(rsLayout);
}

void BufferManager::SetGraphicsRootSignatureLayout(RSLayoutType rsLayout) noexcept {
	m_graphicsRSLayout = std::move(rsLayout);
}

void BufferManager::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

void BufferManager::UpdateModelData(size_t frameIndex) const noexcept {
	size_t offset = 0u;
	static constexpr size_t bufferStride = sizeof(ModelBuffer);

	for (auto& model : m_opaqueModels) {
		ModelBuffer modelBuffer{};
		modelBuffer.textureIndex = model->GetTextureIndex();
		modelBuffer.uvInfo = model->GetUVInfo();
		modelBuffer.modelMatrix = model->GetModelMatrix();
		modelBuffer.modelOffset = model->GetModelOffset();
		modelBuffer.boundingBox = model->GetBoundingBox();

		memcpy(
			m_modelBuffers.GetCPUWPointer(frameIndex) + offset, &modelBuffer,
			bufferStride
		);

		offset += bufferStride;
	}
}
