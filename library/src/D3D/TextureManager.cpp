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

void TextureManager::SetLocalTextureDescriptor(
	D3D12_CPU_DESCRIPTOR_HANDLE unboundHandle, UINT inactiveIndex
) {
	constexpr D3D12_DESCRIPTOR_RANGE_TYPE type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	// The local heap will have a binding for each descriptor type.
	std::vector<UINT>& inactiveDescDetails     = GetInactiveDetails<type>();
	UINT& localDescCount                       = GetLocalDescCount<type>();

	// We add any new items to the end of the inactive indices. The actual heap might
	// be able to house more items, as we will erase the inactive index when it is active
	// so let's check if the buffer can house the new number of inactive indices. If not
	// recreate the heap to be bigger.
	auto localDescIndex = static_cast<UINT>(std::size(inactiveDescDetails));

	const UINT requiredCount = localDescIndex + 1u;

	if (localDescCount < requiredCount)
	{
		localDescCount += s_localDescriptorCount;

		if (m_localDescHeap.Get() != nullptr)
		{
			D3DDescriptorHeap newLocalHeap{
				m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
			};

			newLocalHeap.Create(localDescCount);

			// Since we can't put samplers in an SRV_CBV_UAV heap, this heap
			// should only have textures. So, copying the whole old heap
			// should be fine.
			newLocalHeap.CopyHeap(m_localDescHeap);

			m_localDescHeap = std::move(newLocalHeap);
		}
		else
			m_localDescHeap.Create(localDescCount);
	}

	m_localDescHeap.CopyDescriptor(unboundHandle, localDescIndex);

	inactiveDescDetails.emplace_back(inactiveIndex);
}

std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> TextureManager::GetLocalTextureDescriptor(
	UINT inactiveIndex
) noexcept {
	constexpr D3D12_DESCRIPTOR_RANGE_TYPE type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	std::vector<UINT>& inactiveDescDetails     = GetInactiveDetails<type>();

	auto result = std::ranges::find(inactiveDescDetails, inactiveIndex);

	if (result != std::end(inactiveDescDetails))
	{
		const auto localDescIndex = static_cast<UINT>(
			std::distance(std::begin(inactiveDescDetails), result)
			);

		inactiveDescDetails.erase(result);

		return m_localDescHeap.GetCPUHandle(localDescIndex);
	}

	return {};
}

void TextureManager::RemoveLocalTextureDescriptor(UINT inactiveIndex) noexcept
{
	constexpr D3D12_DESCRIPTOR_RANGE_TYPE type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	std::vector<UINT>& inactiveDescDetails     = GetInactiveDetails<type>();

	std::erase(inactiveDescDetails, inactiveIndex);
}
