#ifndef D3D_EXTERNAL_RESOURCE_MANAGER_HPP_
#define D3D_EXTERNAL_RESOURCE_MANAGER_HPP_
#include <vector>
#include <GraphicsTechniqueExtension.hpp>
#include <D3DExternalResourceFactory.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <D3DStagingBufferManager.hpp>
#include <ReusableVector.hpp>
#include <D3DCommandQueue.hpp>

namespace Gaia
{
class D3DExternalResourceManager
{
	using GfxExtensionContainer_t = Callisto::ReusableVector<GraphicsTechniqueExtension>;

public:
	D3DExternalResourceManager(ID3D12Device* device, MemoryManager* memoryManager);

	[[nodiscard]]
	std::uint32_t AddGraphicsTechniqueExtension(GraphicsTechniqueExtension&& extension);

	void UpdateGraphicsTechniqueExtension(size_t index, GraphicsTechniqueExtension&& extension);

	void RemoveGraphicsTechniqueExtension(std::uint32_t index) noexcept;

	void UpdateDescriptor(
		std::vector<D3DDescriptorManager>& descriptorManagers,
		const ExternalBufferBindingDetails& bindingDetails
	) const;

	void UploadExternalBufferGPUOnlyData(
		StagingBufferManager& stagingBufferManager,
		Callisto::TemporaryDataBufferGPU& tempGPUBuffer, std::uint32_t externalBufferIndex,
		std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes, size_t dstBufferOffset
	) const;

	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes,
		Callisto::TemporaryDataBufferGPU& tempGPUBuffer
	);

	void CopyQueuedBuffers(const D3DCommandList& copyCmdList) noexcept;

	void SetGraphicsDescriptorLayout(std::vector<D3DDescriptorManager>& descriptorManagers);

	[[nodiscard]]
	auto&& GetResourceFactory(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(self.m_resourceFactory);
	}

private:
	struct GPUCopyDetails
	{
		std::uint32_t srcIndex;
		std::uint32_t dstIndex;
		UINT64        srcOffset;
		UINT64        srcSize;
		UINT64        dstOffset;
	};

private:
	void OnGfxExtensionDeletion(const GraphicsTechniqueExtension& gfxExtension);

	void UpdateDescriptor(
		D3DDescriptorManager& descriptorManager,
		const ExternalBufferBindingDetails& bindingDetails
	) const;

private:
	D3DExternalResourceFactory  m_resourceFactory;
	GfxExtensionContainer_t     m_gfxExtensions;
	std::vector<GPUCopyDetails> m_copyQueueDetails;

	static constexpr size_t s_externalBufferRegisterSpace = 2u;

public:
	D3DExternalResourceManager(const D3DExternalResourceManager&) = delete;
	D3DExternalResourceManager& operator=(const D3DExternalResourceManager&) = delete;

	D3DExternalResourceManager(D3DExternalResourceManager&& other) noexcept
		: m_resourceFactory{ std::move(other.m_resourceFactory) },
		m_gfxExtensions{ std::move(other.m_gfxExtensions) },
		m_copyQueueDetails{ std::move(other.m_copyQueueDetails) }
	{}
	D3DExternalResourceManager& operator=(D3DExternalResourceManager&& other) noexcept
	{
		m_resourceFactory  = std::move(other.m_resourceFactory);
		m_gfxExtensions    = std::move(other.m_gfxExtensions);
		m_copyQueueDetails = std::move(other.m_copyQueueDetails);

		return *this;
	}
};
}
#endif
