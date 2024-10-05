#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_
#include <D3DResources.hpp>
#include <StagingBufferManager.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <D3DCommandQueue.hpp>
#include <TemporaryDataBuffer.hpp>
#include <deque>
#include <optional>
#include <Texture.hpp>

// This class will store the texture. A texture added here will also be added to the manager at first.
// But can be removed and re-added.
class TextureStorage
{
	static constexpr size_t s_defaultSamplerIndex = 0u;
public:
	TextureStorage(ID3D12Device* device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_textures{}, m_samplers{}, m_availableTextureIndices{},
		m_availableSamplerIndices{}, m_transitionQueue{}, m_textureBindingIndices{},
		m_samplerBindingIndices{}
	{
		m_samplers.emplace_back(SamplerBuilder{}.Get());
	}

	[[nodiscard]]
	size_t AddTexture(
		STexture&& texture, StagingBufferManager& stagingBufferManager,
		TemporaryDataBufferGPU& tempBuffer, bool msaa = false
	);
	[[nodiscard]]
	size_t AddSampler(const SamplerBuilder& builder);

	void RemoveTexture(size_t index);
	void RemoveSampler(size_t index);

	void SetTextureBindingIndex(size_t textureIndex, UINT bindingIndex) noexcept
	{
		SetBindingIndex(textureIndex, bindingIndex, m_textureBindingIndices);
	}
	void SetSamplerBindingIndex(size_t samplerIndex, UINT bindingIndex) noexcept
	{
		SetBindingIndex(samplerIndex, bindingIndex, m_samplerBindingIndices);
	}

	[[nodiscard]]
	const Texture& Get(size_t index) const noexcept
	{
		return m_textures[index];
	}
	[[nodiscard]]
	Texture const* GetPtr(size_t index) const noexcept
	{
		return &m_textures[index];
	}

	[[nodiscard]]
	UINT GetTextureBindingIndex(size_t textureIndex) const noexcept
	{
		return GetBindingIndex(textureIndex, m_textureBindingIndices);
	}
	[[nodiscard]]
	UINT GetSamplerBindingIndex(size_t samplerIndex) const noexcept
	{
		return GetBindingIndex(samplerIndex, m_samplerBindingIndices);
	}
	[[nodiscard]]
	static constexpr UINT GetDefaultSamplerIndex() noexcept
	{
		return static_cast<UINT>(s_defaultSamplerIndex);
	}

	[[nodiscard]]
	const D3D12_SAMPLER_DESC& GetDefaultSampler() const noexcept
	{
		return m_samplers[s_defaultSamplerIndex];
	}

	[[nodiscard]]
	const D3D12_SAMPLER_DESC& GetSampler(size_t index) const noexcept
	{
		return m_samplers[index];
	}

	// I could have used the AcquireOwnership function to do the layout transition. But there are
	// two reasons, well one in this case to make this extra transition. If the resource has shared
	// ownership, the AcquireOwnership function wouldn't be called. In this class all of the textures
	// have exclusive ownership. But if the transfer and graphics queues have the same family, then
	// the ownership transfer wouldn't be necessary.
	void TransitionQueuedTextures(const D3DCommandList& graphicsCmdList);

private:
	[[nodiscard]]
	static UINT GetBindingIndex(
		size_t index, const std::vector<UINT>& bindingIndices
	) noexcept {
		// The plan is to not initialise the bindingIndices container, if we
		// aren't using the remove and re-adding binding feature. As an element will be
		// added initially and if that feature isn't used, then storing the indices will
		// be just a waste of space. So, if the bindingIndices isn't populated, that
		// will mean the every single element here is also bound. So, their indices should
		// be the same.
		if (std::size(bindingIndices) > index)
			return bindingIndices[index];
		else
			return static_cast<UINT>(index);
	}

	static void SetBindingIndex(
		size_t index, UINT bindingIndex, std::vector<UINT>& bindingIndices
	) noexcept;

private:
	ID3D12Device*                  m_device;
	MemoryManager*                 m_memoryManager;
	// The TextureView objects need to have the same address until their data is copied.
	// For the transitionQueue member and also the StagingBufferManager.
	std::deque<Texture>            m_textures;
	std::deque<D3D12_SAMPLER_DESC> m_samplers;
	std::vector<bool>              m_availableTextureIndices;
	std::vector<bool>              m_availableSamplerIndices;
	std::queue<Texture const*>     m_transitionQueue;
	std::vector<UINT>		       m_textureBindingIndices;
	std::vector<UINT>              m_samplerBindingIndices;

	static constexpr DXGI_FORMAT s_textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

public:
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;

	TextureStorage(TextureStorage&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_textures{ std::move(other.m_textures) },
		m_samplers{ std::move(other.m_samplers) },
		m_availableTextureIndices{ std::move(other.m_availableTextureIndices) },
		m_availableSamplerIndices{ std::move(other.m_availableSamplerIndices) },
		m_transitionQueue{ std::move(other.m_transitionQueue) },
		m_textureBindingIndices{ std::move(other.m_textureBindingIndices) },
		m_samplerBindingIndices{ std::move(other.m_samplerBindingIndices) }
	{}
	TextureStorage& operator=(TextureStorage&& other) noexcept
	{
		m_device                  = other.m_device;
		m_memoryManager           = other.m_memoryManager;
		m_textures                = std::move(other.m_textures);
		m_samplers                = std::move(other.m_samplers);
		m_availableTextureIndices = std::move(other.m_availableTextureIndices);
		m_availableSamplerIndices = std::move(other.m_availableSamplerIndices);
		m_transitionQueue         = std::move(other.m_transitionQueue);
		m_textureBindingIndices   = std::move(other.m_textureBindingIndices);
		m_samplerBindingIndices   = std::move(other.m_samplerBindingIndices);

		return *this;
	}
};

// This class will decide which of the textures will be bound to the pipeline. Every texture from
// the TextureStorage might not be bound at the same time.
class TextureManager
{
	static constexpr std::uint32_t s_textureDescriptorCount
		= std::numeric_limits<std::uint16_t>::max();
	static constexpr std::uint32_t s_samplerDescriptorCount
		= std::numeric_limits<std::uint8_t>::max();

public:
	TextureManager()
		: m_inactiveTextureIndices{}, m_inactiveSamplerIndices{},
		m_availableIndicesTextures(s_textureDescriptorCount, true),
		m_availableIndicesSamplers{}
	{}

	void SetDescriptorLayout(
		D3DDescriptorManager& descriptorManager, size_t texturesRegisterSlot,
		size_t textureRegisterSpace
	) const noexcept;
	void SetDescriptorTable(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t texturesRegisterSlot,
		size_t textureRegisterSpace
	) const;
	// TODO: Add another of these functions for the samplers.

private:
	[[nodiscard]]
	static std::optional<UINT> FindFreeIndex(
		const std::vector<bool>& availableIndices
	) noexcept;

private:
	std::vector<UINT> m_inactiveTextureIndices;
	std::vector<UINT> m_inactiveSamplerIndices;
	std::vector<bool> m_availableIndicesTextures;
	std::vector<bool> m_availableIndicesSamplers;

private:
	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	const std::vector<bool>& GetAvailableIndices() const noexcept
	{
		if constexpr (type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
			return m_availableIndicesSamplers;
		else
			return m_availableIndicesTextures;
	}
	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	std::vector<bool>& GetAvailableIndices() noexcept
	{
		if constexpr (type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
			return m_availableIndicesSamplers;
		else
			return m_availableIndicesTextures;
	}

public:
	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	[[nodiscard]]
	std::optional<UINT> GetFreeGlobalDescriptorIndex() const noexcept
	{
		const std::vector<bool>& availableIndices = GetAvailableIndices<type>();

		return FindFreeIndex(availableIndices);
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	void SetAvailableIndex(UINT descriptorIndex, bool availablity) noexcept
	{
		std::vector<bool>& availableIndices = GetAvailableIndices<type>();

		if (std::size(availableIndices) > descriptorIndex)
			availableIndices[static_cast<size_t>(descriptorIndex)] = availablity;
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	void IncreaseAvailableIndices() noexcept
	{
		if constexpr (type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
		{
			const size_t newSize = std::size(m_availableIndicesSamplers) + s_samplerDescriptorCount;
			m_availableIndicesSamplers.resize(newSize, true);
		}
		else
		{
			const size_t newSize = std::size(m_availableIndicesTextures) + s_textureDescriptorCount;
			m_availableIndicesTextures.resize(newSize, true);
		}
	}

public:
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	TextureManager(TextureManager&& other) noexcept
		: m_inactiveTextureIndices{ std::move(other.m_inactiveTextureIndices) },
		m_inactiveSamplerIndices{ std::move(other.m_inactiveSamplerIndices) },
		m_availableIndicesTextures{ std::move(other.m_availableIndicesTextures) },
		m_availableIndicesSamplers{ std::move(other.m_availableIndicesSamplers) }
	{}
	TextureManager& operator=(TextureManager&& other) noexcept
	{
		m_inactiveTextureIndices   = std::move(other.m_inactiveTextureIndices);
		m_inactiveSamplerIndices   = std::move(other.m_inactiveSamplerIndices);
		m_availableIndicesTextures = std::move(other.m_availableIndicesTextures);
		m_availableIndicesSamplers = std::move(other.m_availableIndicesSamplers);

		return *this;
	}
};
#endif
