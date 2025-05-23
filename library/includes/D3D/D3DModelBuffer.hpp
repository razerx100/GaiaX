#ifndef D3D_MODEL_BUFFER_HPP_
#define D3D_MODEL_BUFFER_HPP_
#include <ModelContainer.hpp>
#include <D3DResources.hpp>
#include <D3DDescriptorHeapManager.hpp>

namespace Gaia
{
class ModelBuffers
{
	using ModelContainer_t = std::shared_ptr<ModelContainer>;

public:
	ModelBuffers(
		ID3D12Device* device, MemoryManager* memoryManager, std::uint32_t frameCount
	) : m_modelContainer{},
		m_vertexModelBuffers{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
		m_pixelModelBuffers{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
		m_modelBuffersInstanceSize{ 0u }, m_modelBuffersPixelInstanceSize{ 0u },
		m_bufferInstanceCount{ frameCount }
	{}

	void SetModelContainer(std::shared_ptr<ModelContainer> modelContainer) noexcept
	{
		m_modelContainer = std::move(modelContainer);
	}

	void ExtendModelBuffers();

	void SetDescriptor(
		D3DDescriptorManager& descriptorManager, UINT64 frameIndex, size_t registerSlot,
		size_t registerSpace, bool graphicsQueue
	) const;
	void SetPixelDescriptor(
		D3DDescriptorManager& descriptorManager, UINT64 frameIndex, size_t registerSlot,
		size_t registerSpace
	) const;

	void Update(UINT64 bufferIndex) const noexcept;

	[[nodiscard]]
	std::uint32_t GetInstanceCount() const noexcept { return m_bufferInstanceCount; }

private:
	struct ModelVertexData
	{
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMMATRIX normalMatrix;
		DirectX::XMFLOAT3 modelOffset;
		// The materialIndex must be grabbed from the z component.
		std::uint32_t     materialIndex;
		std::uint32_t     meshIndex;
		float             modelScale;
		// This struct is 16 bytes aligned thanks to the Matrix. So, putting the correct
		// amount of padding. Just to make it more obvious though, as it would have been
		// put anyway. Structured Buffer doesn't require any specific alignment.
		std::uint32_t     padding[2];
	};

	struct ModelPixelData
	{
		UVInfo        diffuseTexUVInfo;
		UVInfo        specularTexUVInfo;
		std::uint32_t diffuseTexIndex;
		std::uint32_t specularTexIndex;
		float         padding[2]; // Needs to be 16bytes aligned.
	};

private:
	[[nodiscard]]
	static consteval size_t GetVertexStride() noexcept { return sizeof(ModelVertexData); }
	[[nodiscard]]
	static consteval size_t GetPixelStride() noexcept { return sizeof(ModelPixelData); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t modelCount);

private:
	ModelContainer_t m_modelContainer;
	Buffer           m_vertexModelBuffers;
	Buffer           m_pixelModelBuffers;
	UINT64           m_modelBuffersInstanceSize;
	UINT64           m_modelBuffersPixelInstanceSize;
	std::uint32_t    m_bufferInstanceCount;

public:
	ModelBuffers(const ModelBuffers&) = delete;
	ModelBuffers& operator=(const ModelBuffers&) = delete;

	ModelBuffers(ModelBuffers&& other) noexcept
		: m_modelContainer{ std::move(other.m_modelContainer) },
		m_vertexModelBuffers{ std::move(other.m_vertexModelBuffers) },
		m_pixelModelBuffers{ std::move(other.m_pixelModelBuffers) },
		m_modelBuffersInstanceSize{ other.m_modelBuffersInstanceSize },
		m_modelBuffersPixelInstanceSize{ other.m_modelBuffersPixelInstanceSize },
		m_bufferInstanceCount{ other.m_bufferInstanceCount }
	{}
	ModelBuffers& operator=(ModelBuffers&& other) noexcept
	{
		m_modelContainer                = std::move(other.m_modelContainer);
		m_vertexModelBuffers            = std::move(other.m_vertexModelBuffers);
		m_pixelModelBuffers             = std::move(other.m_pixelModelBuffers);
		m_modelBuffersInstanceSize      = other.m_modelBuffersInstanceSize;
		m_modelBuffersPixelInstanceSize = other.m_modelBuffersPixelInstanceSize;
		m_bufferInstanceCount           = other.m_bufferInstanceCount;

		return *this;
	}
};
}
#endif
