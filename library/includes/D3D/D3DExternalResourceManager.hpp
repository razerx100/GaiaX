#ifndef D3D_EXTERNAL_RESOURCE_MANAGER_HPP_
#define D3D_EXTERNAL_RESOURCE_MANAGER_HPP_
#include <vector>
#include <ExternalResourceManager.hpp>
#include <D3DExternalResourceFactory.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <StagingBufferManager.hpp>
#include <ReusableVector.hpp>
#include <D3DCommandQueue.hpp>

class D3DExternalResourceManager : public ExternalResourceManager
{
	using GfxExtension_t = std::shared_ptr<GraphicsTechniqueExtension>;

public:
	D3DExternalResourceManager(ID3D12Device* device, MemoryManager* memoryManager);

	[[nodiscard]]
	std::uint32_t AddGraphicsTechniqueExtension(
		std::shared_ptr<GraphicsTechniqueExtension> extension
	) override;

	void RemoveGraphicsTechniqueExtension(std::uint32_t index) noexcept override;

	void UpdateDescriptor(
		std::vector<D3DDescriptorManager>& descriptorManagers,
		const ExternalBufferBindingDetails& bindingDetails
	) const;

	void UploadExternalBufferGPUOnlyData(
		StagingBufferManager& stagingBufferManager, TemporaryDataBufferGPU& tempGPUBuffer,
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
		size_t dstBufferOffset
	) const;

	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes,
		TemporaryDataBufferGPU& tempGPUBuffer
	);

	void CopyQueuedBuffers(const D3DCommandList& copyCmdList) noexcept;

	void UpdateExtensionData(size_t frameIndex) const noexcept;

	void SetGraphicsDescriptorLayout(std::vector<D3DDescriptorManager>& descriptorManagers);

	[[nodiscard]]
	ExternalResourceFactory* GetResourceFactory() noexcept override
	{
		return GetD3DResourceFactory();
	}

	[[nodiscard]]
	ExternalResourceFactory const* GetResourceFactory() const noexcept override
	{
		return GetD3DResourceFactory();
	}

	[[nodiscard]]
	D3DExternalResourceFactory* GetD3DResourceFactory() noexcept
	{
		return m_resourceFactory.get();
	}

	[[nodiscard]]
	D3DExternalResourceFactory const* GetD3DResourceFactory() const noexcept
	{
		return m_resourceFactory.get();
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
		D3DDescriptorManager& descriptorManager, const ExternalBufferBindingDetails& bindingDetails
	) const;

private:
	std::unique_ptr<D3DExternalResourceFactory> m_resourceFactory;
	Callisto::ReusableVector<GfxExtension_t>    m_gfxExtensions;
	std::vector<GPUCopyDetails>                 m_copyQueueDetails;

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
#endif
