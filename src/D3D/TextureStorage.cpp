#include <TextureStorage.hpp>
#include <Gaia.hpp>
#include <cstring>
#include <D3DHelperFunctions.hpp>

size_t TextureStorage::AddTexture(
	ID3D12Device* device,
	std::unique_ptr<std::uint8_t> textureDataHandle,
	size_t width, size_t height, bool components16bits
) noexcept {
	DXGI_FORMAT textureFormat = DXGI_FORMAT_UNKNOWN;
	size_t bytesPerPixel = 0u;

	if (components16bits) {
		textureFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
		bytesPerPixel = 8u;
	}
	else {
		textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		bytesPerPixel = 4u;
	}

	m_textureData.emplace_back(textureFormat, width * bytesPerPixel, height);

	m_dataHandles.emplace_back(std::move(textureDataHandle));

	auto [gpuBuffer, uploadBuffer] = Gaia::heapManager->AddTexture(
		device, width, height, bytesPerPixel
	);

	m_gpuBuffers.emplace_back(gpuBuffer);
	m_uploadBuffers.emplace_back(uploadBuffer);

	SharedCPUHandle sharedCPUHandle = Gaia::descriptorTable->ReserveDescriptorsTexture();

	m_cpuHandles.emplace_back(std::move(sharedCPUHandle));

	return std::size(m_cpuHandles) - 1u;
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

void TextureStorage::CopyData(std::atomic_size_t& workCount) noexcept {
	// Copy data
	workCount += 1u;

	Gaia::threadPool->SubmitWork(
		[&] {
			for (size_t index = 0u; index < std::size(m_textureData); ++index) {
				TextureData& texData = m_textureData[index];
				size_t paddedRowPitch = Align(
					texData.rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
				);

				for (size_t columnIndex = 0u; columnIndex < texData.height; ++columnIndex) {
					std::memcpy(
						m_uploadBuffers[index]->GetCPUWPointer()
						+ (columnIndex * paddedRowPitch),
						m_dataHandles[index].get() + (columnIndex * texData.rowPitch),
						texData.rowPitch
					);
				}
			}

			--workCount;
		}
	);
}

void TextureStorage::ReleaseUploadBuffer() noexcept {
	m_textureData = std::vector<TextureData>();
	m_uploadBuffers = std::vector<D3DCPUWResourceShared>();
	m_cpuHandles = std::vector<SharedCPUHandle>();
}
