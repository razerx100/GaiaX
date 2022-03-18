#include <TextureStorage.hpp>
#include <InstanceManager.hpp>
#include <CRSMath.hpp>
#include <VenusInstance.hpp>
#include <cstring>
#include <GraphicsEngineDx12.hpp>

size_t TextureStorage::AddTexture(
	ID3D12Device* device,
	const void* data, size_t rowPitch, size_t rows
) noexcept {
	m_textureData.emplace_back(data, rowPitch * rows);

	auto [gpuBuffer, uploadBuffer] =
		HeapManagerInst::GetRef()->AddTexture(device, rowPitch, rows);

	m_gpuBuffers.emplace_back(gpuBuffer);
	m_uploadBuffers.emplace_back(uploadBuffer);

	auto [sharedIndex, sharedCPUHandle] = DescTableManInst::GetRef()->GetTextureIndex();

	m_cpuHandles.emplace_back(sharedCPUHandle);
	m_textureIndices.emplace_back(sharedIndex);

	return m_textureIndices.size() - 1u;
}

void TextureStorage::CreateBufferViews(ID3D12Device* device) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = GraphicsEngineDx12::RENDER_FORMAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1u;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
	for (size_t index = 0u; index < m_cpuHandles.size(); ++index) {
		cpuHandle.ptr = *m_cpuHandles[index];

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

	GetVenusInstance()->SubmitWork(
		[&] {
			for (size_t index = 0u; index < m_textureData.size(); ++index) {
				TextureData& textureData = m_textureData[index];

				std::memcpy(
					m_uploadBuffers[index]->GetCPUHandle(),
					textureData.data, textureData.textureSize
				);
			}

			--workCount;
		}
	);
}

void TextureStorage::ReleaseUploadBuffer() noexcept {
	m_uploadBuffers = std::vector<std::shared_ptr<IUploadBuffer>>();
	m_cpuHandles = std::vector<SharedCPUHandle>();
}
