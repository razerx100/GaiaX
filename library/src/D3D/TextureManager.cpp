#include <TextureManager.hpp>
#include <D3DResourceBarrier.hpp>

// Texture storage
void TextureStorage::SetBindingIndex(
	size_t index, UINT bindingIndex, std::vector<UINT>& bindingIndices
) noexcept {
	if (std::size(bindingIndices) <= index)
	{
		bindingIndices.resize(index + 1u);
	}

	bindingIndices[index] = bindingIndex;
}

size_t TextureStorage::AddTexture(
	STexture&& texture, StagingBufferManager& stagingBufferManager, TemporaryDataBufferGPU& tempBuffer,
	bool msaa/* = false */
) {
	const size_t index = m_textures.Add(Texture{ m_device, m_memoryManager, D3D12_HEAP_TYPE_DEFAULT });

	Texture* texturePtr = &m_textures[index];

	texturePtr->Create2D(
		texture.width, texture.height, 1u, s_textureFormat, D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_FLAG_NONE, msaa
	);

	stagingBufferManager.AddTexture(std::move(texture.data), texturePtr, tempBuffer);

	// Should be fine because of the deque.
	m_transitionQueue.push(texturePtr);

	return index;
}

size_t TextureStorage::AddSampler(const SamplerBuilder& builder)
{
	return m_samplers.Add(builder.Get());
}

void TextureStorage::TransitionQueuedTextures(const D3DCommandList& graphicsCmdList)
{
	while (!std::empty(m_transitionQueue))
	{
		Texture const* texturePtr = m_transitionQueue.front();
		m_transitionQueue.pop();

		D3DResourceBarrier{}.AddBarrier(
			ResourceBarrierBuilder{}
			.Transition(
				texturePtr->Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			)
		).RecordBarriers(graphicsCmdList.Get());
	}
}

void TextureStorage::RemoveTexture(size_t index)
{
	m_textures[index].Destroy();
	m_textures.RemoveElement(index);
}

void TextureStorage::RemoveSampler(size_t index)
{
	if (index != s_defaultSamplerIndex)
		m_samplers.RemoveElement(index);

	// Don't need to destroy this like textures, as it doesn't require any buffer allocations.
}

void TextureStorage::SetTextureCacheDetails(UINT textureIndex, UINT localDescIndex) noexcept
{
	m_textureCacheDetails.emplace_back(
		TextureCacheDetails
		{
			.textureIndex   = textureIndex,
			.localDescIndex = localDescIndex
		}
	);
}

std::optional<UINT> TextureStorage::GetAndRemoveTextureLocalDescIndex(UINT textureIndex) noexcept
{
	auto result = std::ranges::find_if(
		m_textureCacheDetails,
		[textureIndex]
		(const TextureCacheDetails& cacheDetails)
		{
			return cacheDetails.textureIndex == textureIndex;
		}
	);

	std::optional<UINT> oLocalDescIndex{};

	if (result != std::end(m_textureCacheDetails))
		oLocalDescIndex = result->localDescIndex;

	return oLocalDescIndex;
}

std::vector<UINT> TextureStorage::GetAndRemoveTextureCacheDetails(UINT textureIndex) noexcept
{
	std::vector<UINT> localDescDetails{};

	std::erase_if(
		m_textureCacheDetails,
		[textureIndex, &localDescDetails](const TextureCacheDetails& cacheDetails)
		{
			const bool doesMatch = textureIndex == cacheDetails.textureIndex;

			if (doesMatch)
				localDescDetails.emplace_back(cacheDetails.localDescIndex);

			return doesMatch;
		}
	);

	return localDescDetails;
}

// Texture Manager
void TextureManager::SetDescriptorLayout(
	D3DDescriptorManager& descriptorManager, size_t texturesRegisterSlot,
	size_t textureRegisterSpace
) const noexcept {
	const auto textureDescCount = static_cast<UINT>(std::size(m_availableIndicesTextures));

	if (textureDescCount)
		descriptorManager.AddSRVTable(
			texturesRegisterSlot, textureRegisterSpace, textureDescCount,
			D3D12_SHADER_VISIBILITY_PIXEL, true
		);
}

void TextureManager::SetDescriptorTable(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t texturesRegisterSlot,
	size_t textureRegisterSpace
) const {
	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
		descriptorManager.SetDescriptorTableSRV(texturesRegisterSlot, textureRegisterSpace, 0u, true);
}
