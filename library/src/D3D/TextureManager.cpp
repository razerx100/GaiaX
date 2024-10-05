#include <TextureManager.hpp>
#include <D3DResourceBarrier.hpp>

template<typename T>
[[nodiscard]]
static std::pair<size_t, T*> AddResource(
	std::vector<bool>& availableIndices, std::deque<T>& resources, T&& resource
) noexcept {
	auto index     = std::numeric_limits<size_t>::max();
	T* resourcePtr = nullptr;

	auto result = std::ranges::find(availableIndices, true);

	if (result != std::end(availableIndices))
	{
		index       = static_cast<size_t>(std::distance(std::begin(availableIndices), result));
		resourcePtr = &resources[index];
	}
	else
	{
		index          = std::size(resources);
		T& returnedRef = resources.emplace_back(std::move(resource));
		availableIndices.emplace_back(false);
		resourcePtr    = &returnedRef;
	}

	return { index, resourcePtr };
}

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
	auto [index, texturePtr] = AddResource(
		m_availableTextureIndices, m_textures,
		Texture{ m_device, m_memoryManager, D3D12_HEAP_TYPE_DEFAULT }
	);

	texturePtr->Create2D(
		texture.width, texture.height, 1u, s_textureFormat, D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_FLAG_NONE, msaa
	);

	stagingBufferManager.AddTexture(std::move(texture.data), texturePtr, tempBuffer);

	m_transitionQueue.push(texturePtr);

	return index;
}

size_t TextureStorage::AddSampler(const SamplerBuilder& builder)
{
	auto [index, samplerPtr] = AddResource(m_availableSamplerIndices, m_samplers, builder.Get());

	return index;
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
	m_availableTextureIndices[index] = true;
	m_textures[index].Destroy();
}

void TextureStorage::RemoveSampler(size_t index)
{
	if (index != s_defaultSamplerIndex)
		m_availableSamplerIndices[index] = true;
	// Don't need to destroy this like textures, as it doesn't require any buffer allocations.
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
	for (auto& descriptorManager : descriptorManagers)
		descriptorManager.SetDescriptorTableSRV(texturesRegisterSlot, textureRegisterSpace, 0u, true);
}

std::optional<UINT> TextureManager::FindFreeIndex(
	const std::vector<bool>& availableIndices
) noexcept {
	auto result = std::ranges::find(availableIndices, true);

	if (result != std::end(availableIndices))
		return static_cast<UINT>(std::distance(std::begin(availableIndices), result));

	return {};
}
