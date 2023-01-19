#include <BufferManager.hpp>
#include <ranges>
#include <algorithm>
#include <Gaia.hpp>

#include <CameraManager.hpp>

BufferManager::BufferManager(const Args& arguments)
	: m_cameraBuffer{}, m_pixelDataBuffer{},
	m_modelBuffers{ ResourceType::cpuWrite, DescriptorType::SRV },
	m_materialBuffers{ ResourceType::cpuWrite, DescriptorType::SRV },
	m_lightBuffers{ ResourceType::cpuWrite, DescriptorType::SRV },
	m_frameCount{ arguments.frameCount.value() } {}

void BufferManager::ReserveBuffers(ID3D12Device* device) noexcept {
	// Camera
	size_t cameraBufferSize = sizeof(DirectX::XMMATRIX) * 2u;
	constexpr size_t constantBufferAlignment = 256u;
	size_t cameraOffset =
		Gaia::Resources::cpuWriteBuffer->ReserveSpaceSuballocatedAndGetOffset(
			cameraBufferSize, m_frameCount, constantBufferAlignment
		);
	m_cameraBuffer.SetAddressesStart(cameraOffset, cameraBufferSize, constantBufferAlignment);

	// Pixel Data
	constexpr size_t pixelDataBufferSize = sizeof(PixelData);
	size_t pixelDataOffset =
		Gaia::Resources::cpuWriteBuffer->ReserveSpaceSuballocatedAndGetOffset(
			pixelDataBufferSize, m_frameCount, constantBufferAlignment
		);
	m_pixelDataBuffer.SetAddressesStart(
		pixelDataOffset, pixelDataBufferSize, constantBufferAlignment
	);

	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Model Data
	const size_t modelBufferDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);
	const auto modelCount = static_cast<UINT>(std::size(m_opaqueModels));

	m_modelBuffers.SetDescriptorOffset(modelBufferDescriptorOffset, descriptorSize);
	m_modelBuffers.SetBufferInfo(
		device, static_cast<UINT>(sizeof(ModelBuffer)), modelCount, m_frameCount
	);

	// Material Data
	const size_t materialBufferDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);

	m_materialBuffers.SetDescriptorOffset(materialBufferDescriptorOffset, descriptorSize);
	m_materialBuffers.SetBufferInfo(
		device, static_cast<UINT>(sizeof(MaterialBuffer)), modelCount, m_frameCount
	);

	// Light Data
	const size_t lightBufferDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);

	m_lightBuffers.SetDescriptorOffset(lightBufferDescriptorOffset, descriptorSize);
	m_lightBuffers.SetBufferInfo(
		device, static_cast<UINT>(sizeof(LightBuffer)),
		static_cast<UINT>(std::size(m_lightModelIndices)), m_frameCount
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
	m_materialBuffers.CreateDescriptorView(
		device, uploadDescriptorStart, gpuDescriptorStart, D3D12_RESOURCE_STATE_GENERIC_READ
	);
	m_lightBuffers.CreateDescriptorView(
		device, uploadDescriptorStart, gpuDescriptorStart, D3D12_RESOURCE_STATE_GENERIC_READ
	);

	SetMemoryAddresses();
}

void BufferManager::SetMemoryAddresses() noexcept {
	std::uint8_t* cpuOffset = Gaia::Resources::cpuWriteBuffer->GetCPUStartAddress();
	D3D12_GPU_VIRTUAL_ADDRESS gpuOffset = Gaia::Resources::cpuWriteBuffer->GetGPUStartAddress();

	m_cameraBuffer.UpdateCPUAddressStart(cpuOffset);
	m_cameraBuffer.UpdateGPUAddressStart(gpuOffset);
	m_pixelDataBuffer.UpdateCPUAddressStart(cpuOffset);
	m_pixelDataBuffer.UpdateGPUAddressStart(gpuOffset);
}

void BufferManager::Update(size_t frameIndex) const noexcept {
	UpdateCameraData(frameIndex);
	UpdatePerModelData(frameIndex);
	UpdateLightData(frameIndex);
	UpdatePixelData(frameIndex);
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

void BufferManager::BindPixelOnlyBuffers(
	ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex
) const noexcept {
	static constexpr auto materialTypeIndex = static_cast<size_t>(RootSigElement::MaterialData);
	graphicsCmdList->SetGraphicsRootDescriptorTable(
		m_graphicsRSLayout[materialTypeIndex],
		m_materialBuffers.GetGPUDescriptorHandle(frameIndex)
	);

	static constexpr auto lightTypeIndex = static_cast<size_t>(RootSigElement::LightData);
	graphicsCmdList->SetGraphicsRootDescriptorTable(
		m_graphicsRSLayout[lightTypeIndex], m_lightBuffers.GetGPUDescriptorHandle(frameIndex)
	);

	static constexpr auto pixelDataTypeIndex = static_cast<size_t>(RootSigElement::PixelData);
	graphicsCmdList->SetGraphicsRootConstantBufferView(
		m_graphicsRSLayout[pixelDataTypeIndex],
		m_pixelDataBuffer.GetGPUAddressStart(frameIndex)
	);
}

void BufferManager::SetComputeRootSignatureLayout(RSLayoutType rsLayout) noexcept {
	m_computeRSLayout = std::move(rsLayout);
}

void BufferManager::SetGraphicsRootSignatureLayout(RSLayoutType rsLayout) noexcept {
	m_graphicsRSLayout = std::move(rsLayout);
}

void BufferManager::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	for (size_t index = 0u; index < std::size(models); ++index)
		if (models[index]->IsLightSource())
			m_lightModelIndices.emplace_back(std::size(m_opaqueModels) + index);

	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

void BufferManager::UpdateCameraData(size_t bufferIndex) const noexcept {
	std::uint8_t* cameraCpuHandle = m_cameraBuffer.GetCPUAddressStart(bufferIndex);

	Gaia::cameraManager->CopyData(cameraCpuHandle);
}

void BufferManager::UpdatePerModelData(size_t bufferIndex) const noexcept {
	size_t modelOffset = 0u;
	std::uint8_t* modelBufferOffset = m_modelBuffers.GetCPUWPointer(bufferIndex);

	size_t materialOffset = 0u;
	std::uint8_t* materialBufferOffset = m_materialBuffers.GetCPUWPointer(bufferIndex);

	for (auto& model : m_opaqueModels) {
		ModelBuffer modelBuffer{
			.uvInfo = model->GetUVInfo(),
			.modelMatrix = model->GetModelMatrix(),
			.textureIndex = model->GetTextureIndex(),
			.modelOffset = model->GetModelOffset(),
			.boundingBox = model->GetBoundingBox()
		};
		CopyStruct(modelBuffer, modelBufferOffset, modelOffset);

		const auto& modelMaterial = model->GetMaterial();

		MaterialBuffer material{
			.ambient = modelMaterial.ambient,
			.diffuse = modelMaterial.diffuse,
			.specular = modelMaterial.specular,
			.shininess = modelMaterial.shininess
		};
		CopyStruct(material, materialBufferOffset, materialOffset);
	}
}

void BufferManager::UpdateLightData(size_t bufferIndex) const noexcept {
	size_t offset = 0u;
	std::uint8_t* lightBufferOffset = m_lightBuffers.GetCPUWPointer(bufferIndex);

	const DirectX::XMMATRIX viewMatrix = Gaia::sharedData->GetViewMatrix();

	for (auto& lightIndex : m_lightModelIndices) {
		auto& model = m_opaqueModels[lightIndex];
		const auto& modelMaterial = model->GetMaterial();

		LightBuffer light{
			.ambient = modelMaterial.ambient,
			.diffuse = modelMaterial.diffuse,
			.specular = modelMaterial.specular
		};

		DirectX::XMFLOAT3 worldPosition = model->GetModelOffset();
		DirectX::XMVECTOR viewPosition = DirectX::XMVector3Transform(
			DirectX::XMLoadFloat3(&worldPosition), viewMatrix
		);
		DirectX::XMStoreFloat3(&light.position, viewPosition);

		CopyStruct(light, lightBufferOffset, offset);
	}
}

void BufferManager::UpdatePixelData(size_t bufferIndex) const noexcept {
	std::uint8_t* pixelDataOffset = m_pixelDataBuffer.GetCPUAddressStart(bufferIndex);
	const auto lightCount = static_cast<std::uint32_t>(std::size(m_lightModelIndices));

	memcpy(pixelDataOffset, &lightCount, sizeof(PixelData));
}
