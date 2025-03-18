#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_
#include <D3DResources.hpp>
#include <StagingBufferManager.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <D3DCommandQueue.hpp>
#include <TemporaryDataBuffer.hpp>
#include <ReusableVector.hpp>
#include <deque>
#include <optional>
#include <Texture.hpp>

// This class will store the texture. A texture added here will also be added to the manager at first.
// But can be removed and re-added.
class TextureStorage
{
	inline static size_t s_defaultSamplerIndex = 0u;
public:
	TextureStorage(ID3D12Device* device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_textures{}, m_samplers{}, m_transitionQueue{}, m_textureBindingIndices{},
		m_samplerBindingIndices{}, m_textureCacheDetails{}
	{}

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

	void SetTextureCacheDetails(UINT textureIndex, UINT localDescIndex) noexcept;

	[[nodiscard]]
	std::optional<UINT> GetAndRemoveTextureLocalDescIndex(UINT textureIndex) noexcept;

	[[nodiscard]]
	std::vector<UINT> GetAndRemoveTextureCacheDetails(UINT textureIndex) noexcept;

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
	struct TextureCacheDetails
	{
		UINT textureIndex;
		UINT localDescIndex;

		constexpr bool operator==(const TextureCacheDetails& other) const noexcept
		{
			return textureIndex   == other.textureIndex
				&& localDescIndex == other.localDescIndex;
		}
	};

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
	ID3D12Device*                     m_device;
	MemoryManager*                    m_memoryManager;
	// The TextureView objects need to have the same address until their data is copied.
	// For the transitionQueue member and also the StagingBufferManager.
	ReusableDeque<Texture>            m_textures;
	ReusableDeque<D3D12_SAMPLER_DESC> m_samplers;
	std::queue<Texture const*>        m_transitionQueue;
	std::vector<UINT>		          m_textureBindingIndices;
	std::vector<UINT>                 m_samplerBindingIndices;

	std::vector<TextureCacheDetails>  m_textureCacheDetails;
	// Sampler cache too at some point?

	static constexpr DXGI_FORMAT s_textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

public:
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;

	TextureStorage(TextureStorage&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_textures{ std::move(other.m_textures) },
		m_samplers{ std::move(other.m_samplers) },
		m_transitionQueue{ std::move(other.m_transitionQueue) },
		m_textureBindingIndices{ std::move(other.m_textureBindingIndices) },
		m_samplerBindingIndices{ std::move(other.m_samplerBindingIndices) },
		m_textureCacheDetails{ std::move(other.m_textureCacheDetails) }
	{}
	TextureStorage& operator=(TextureStorage&& other) noexcept
	{
		m_device                = other.m_device;
		m_memoryManager         = other.m_memoryManager;
		m_textures              = std::move(other.m_textures);
		m_samplers              = std::move(other.m_samplers);
		m_transitionQueue       = std::move(other.m_transitionQueue);
		m_textureBindingIndices = std::move(other.m_textureBindingIndices);
		m_samplerBindingIndices = std::move(other.m_samplerBindingIndices);
		m_textureCacheDetails   = std::move(other.m_textureCacheDetails);

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
	static constexpr UINT s_localDescriptorCount = std::numeric_limits<std::uint8_t>::max();

public:
	TextureManager(ID3D12Device* device)
		: m_device{ device },
		m_availableIndicesTextures{ s_textureDescriptorCount }, m_availableIndicesSamplers{},
		m_localTextureDescHeap{
			device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		}, m_textureCaches{}
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
	ID3D12Device*     m_device;
	IndicesManager    m_availableIndicesTextures;
	IndicesManager    m_availableIndicesSamplers;
	D3DDescriptorHeap m_localTextureDescHeap;
	IndicesManager    m_textureCaches;
	// Need another local heap for the samplers.

private:
	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	[[nodiscard]]
	auto&& GetAvailableBindings(this auto&& self) noexcept
	{
		if constexpr (type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
			return std::forward_like<decltype(self)>(self.m_availableIndicesSamplers);
		else
			return std::forward_like<decltype(self)>(self.m_availableIndicesTextures);
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	[[nodiscard]]
	auto&& GetCaches(this auto&& self) noexcept
	{
		if constexpr (type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
			// This one needs to be changed with the sampler cache when implemented.
			return std::forward_like<decltype(self)>(self.m_textureCaches);
		else
			return std::forward_like<decltype(self)>(self.m_textureCaches);
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	[[nodiscard]]
	auto&& GetLocalDescHeap(this auto&& self) noexcept
	{
		if constexpr (type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
			// This one needs to be changed with the sampler heap when implemented.
			return std::forward_like<decltype(self)>(self.m_localTextureDescHeap);
		else
			return std::forward_like<decltype(self)>(self.m_localTextureDescHeap);
	}

public:
	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	[[nodiscard]]
	std::optional<size_t> GetFreeGlobalDescriptorIndex() const noexcept
	{
		const IndicesManager& availableBindings = GetAvailableBindings<type>();

		return availableBindings.GetFirstAvailableIndex();
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	void SetBindingAvailability(UINT descriptorIndex, bool availability) noexcept
	{
		IndicesManager& availableBindings = GetAvailableBindings<type>();

		if (std::size(availableBindings) > descriptorIndex)
			availableBindings.ToggleAvailability(static_cast<size_t>(descriptorIndex), availability);
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	void IncreaseMaximumBindingCount() noexcept
	{
		if constexpr (type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
		{
			const size_t newSize = std::size(m_availableIndicesSamplers) + s_samplerDescriptorCount;
			m_availableIndicesSamplers.Resize(newSize);
		}
		else
		{
			const size_t newSize = std::size(m_availableIndicesTextures) + s_textureDescriptorCount;
			m_availableIndicesTextures.Resize(newSize);
		}
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	void SetLocalDescriptorAvailability(UINT localDescIndex, bool availability) noexcept
	{
		IndicesManager& localCaches = GetCaches<type>();

		localCaches.ToggleAvailability(localDescIndex, availability);
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	UINT GetFirstFreeLocalDescriptor()
	{
		IndicesManager& localCaches      = GetCaches<type>();

		std::optional<size_t> oFreeIndex = localCaches.GetFirstAvailableIndex();

		if (!oFreeIndex)
		{
			const auto newLocalDescCount = static_cast<UINT>(
				std::size(localCaches) + s_localDescriptorCount
			);

			localCaches.Resize(newLocalDescCount);

			oFreeIndex = localCaches.GetFirstAvailableIndex();

			// Extended the desc heap.
			D3DDescriptorHeap& localDescHeap    = GetLocalDescHeap<type>();

			D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

			if constexpr (type == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
				heapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

			if (localDescHeap.Get() != nullptr)
			{
				D3DDescriptorHeap newLocalHeap{ m_device, heapType, D3D12_DESCRIPTOR_HEAP_FLAG_NONE };

				newLocalHeap.Create(newLocalDescCount);

				newLocalHeap.CopyHeap(localDescHeap);

				localDescHeap = std::move(newLocalHeap);
			}
			else
				localDescHeap.Create(newLocalDescCount);
		}

		return static_cast<UINT>(oFreeIndex.value());
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetLocalDescriptor(UINT localDescIndex) noexcept
	{
		const D3DDescriptorHeap& localDescHeap = GetLocalDescHeap<type>();

		return localDescHeap.GetCPUHandle(localDescIndex);
	}

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	void SetLocalDescriptor(
	D3D12_CPU_DESCRIPTOR_HANDLE unboundHandle, UINT localDescIndex)
	{
		D3DDescriptorHeap& localDescHeap = GetLocalDescHeap<type>();

		localDescHeap.CopyDescriptor(unboundHandle, localDescIndex);
	}

public:
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	TextureManager(TextureManager&& other) noexcept
		: m_device{ other.m_device },
		m_availableIndicesTextures{ std::move(other.m_availableIndicesTextures) },
		m_availableIndicesSamplers{ std::move(other.m_availableIndicesSamplers) },
		m_localTextureDescHeap{ std::move(other.m_localTextureDescHeap) },
		m_textureCaches{ std::move(other.m_textureCaches) }
	{}
	TextureManager& operator=(TextureManager&& other) noexcept
	{
		m_device                   = other.m_device;
		m_availableIndicesTextures = std::move(other.m_availableIndicesTextures);
		m_availableIndicesSamplers = std::move(other.m_availableIndicesSamplers);
		m_localTextureDescHeap     = std::move(other.m_localTextureDescHeap);
		m_textureCaches            = std::move(other.m_textureCaches);

		return *this;
	}
};
#endif
