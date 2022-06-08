#include <TextureStorage.hpp>
#include <Gaia.hpp>
#include <cstring>

size_t TextureStorage::AddTexture(
	ID3D12Device* device,
	std::unique_ptr<std::uint8_t> textureDataHandle,
	size_t width, size_t height, size_t pixelSizeInBytes
) noexcept {
	DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;

	if (pixelSizeInBytes == 16u)
		textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if(pixelSizeInBytes == 4u)
		textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	m_textureData.emplace_back(
		width * pixelSizeInBytes * height, textureFormat
	);

	m_dataHandles.emplace_back(std::move(textureDataHandle));

	auto [gpuBuffer, uploadBuffer] =
		Gaia::heapManager->AddTexture(device, width, height, pixelSizeInBytes);

	m_gpuBuffers.emplace_back(gpuBuffer);
	m_uploadBuffers.emplace_back(uploadBuffer);

	auto [sharedIndex, sharedCPUHandle] =
		Gaia::descriptorTable->GetTextureIndex();

	m_cpuHandles.emplace_back(sharedCPUHandle);
	m_textureIndices.emplace_back(sharedIndex);

	return std::size(m_textureIndices) - 1u;
}

void TextureStorage::CreateBufferViews(ID3D12Device* device) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1u;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
	for (size_t index = 0u; index < std::size(m_cpuHandles); ++index) {
		cpuHandle.ptr = *m_cpuHandles[index];

		srvDesc.Format = m_textureData[index].textureFormat;

		device->CreateShaderResourceView(
			m_gpuBuffers[index]->Get(), &srvDesc, cpuHandle
		);
	}
}

size_t TextureStorage::GetPhysicalIndexFromVirtual(size_t virtualIndex) const noexcept {
	return *m_textureIndices[virtualIndex];
}

void TextureStorage::CopyData(std::atomic_size_t& workCount) noexcept {
	// Copy data
	workCount += 1u;

	Gaia::threadPool->SubmitWork(
		[&] {
			for (size_t index = 0u; index < std::size(m_textureData); ++index)
				std::memcpy(
					m_uploadBuffers[index]->GetCPUHandle(),
					m_dataHandles[index].get(), m_textureData[index].textureSize
				);

			--workCount;
		}
	);
}

void TextureStorage::ReleaseUploadBuffer() noexcept {
	m_textureData = std::vector<TextureData>();
	m_uploadBuffers = std::vector<std::shared_ptr<UploadBuffer>>();
	m_cpuHandles = std::vector<SharedCPUHandle>();
}
