#include <TextureStorage.hpp>
#include <Gaia.hpp>

TextureStorage::TextureStorage() noexcept : m_textureDescriptorStart{} {}

size_t TextureStorage::AddTexture(
	ID3D12Device* device, std::unique_ptr<std::uint8_t> textureDataHandle, size_t width,
	size_t height
) noexcept {
	const size_t relativeTextureOffset =
		Gaia::descriptorTable->ReserveDescriptorsTextureAndGetRelativeOffset();

	auto textureDescriptor =
		std::make_unique<D3DUploadResourceDescriptorView>(DescriptorType::SRV);
	textureDescriptor->SetDescriptorOffset(
		relativeTextureOffset,
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	);
	textureDescriptor->SetTextureInfo(
		device, static_cast<UINT64>(width), static_cast<UINT>(height),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, false
	);

	m_textureHandles.emplace_back(std::move(textureDataHandle));
	m_textureDescriptors.emplace_back(std::move(textureDescriptor));

	return relativeTextureOffset;
}

void TextureStorage::CreateBufferViews(ID3D12Device* device) {
	const D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart =
		Gaia::descriptorTable->GetUploadDescriptorStart();
	const D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart =
		Gaia::descriptorTable->GetGPUDescriptorStart();

	const size_t textureRangeStart = Gaia::descriptorTable->GetTextureRangeStart();

	const size_t descSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_textureDescriptorStart.ptr = gpuDescriptorStart.ptr + (descSize * textureRangeStart);

	for (size_t index = 0u; index < std::size(m_textureDescriptors); ++index) {
		auto& textureDescriptor = m_textureDescriptors[index];

		textureDescriptor->UpdateDescriptorOffset(textureRangeStart);
		textureDescriptor->CreateDescriptorView(
			device, uploadDescriptorStart, gpuDescriptorStart,
			D3D12_RESOURCE_STATE_COPY_DEST
		);

		const D3D12_RESOURCE_DESC textureDesc = textureDescriptor->GetResourceDesc();
		Gaia::Resources::uploadContainer->AddMemory(
			m_textureHandles[index].get(), textureDescriptor->GetFirstCPUWPointer(),
			textureDesc.Width * 4u, textureDesc.Height
		);
	}
}

void TextureStorage::BindTextures(ID3D12GraphicsCommandList* graphicsList) const noexcept {
	static constexpr size_t texturesIndex = static_cast<size_t>(RootSigElement::Textures);

	graphicsList->SetGraphicsRootDescriptorTable(
		m_graphicsRSLayout[texturesIndex], m_textureDescriptorStart
	);
}

void TextureStorage::RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept {
	for (auto& textureDesc : m_textureDescriptors)
		textureDesc->RecordResourceUpload(copyList);
}

void TextureStorage::ReleaseUploadResource() noexcept {
	for (auto& textureDesc : m_textureDescriptors)
		textureDesc->ReleaseUploadResource();

	m_textureHandles = std::vector<std::unique_ptr<std::uint8_t>>();
}

void TextureStorage::SetGraphicsRootSignatureLayout(std::vector<UINT> rsLayout) noexcept {
	m_graphicsRSLayout = std::move(rsLayout);
}
