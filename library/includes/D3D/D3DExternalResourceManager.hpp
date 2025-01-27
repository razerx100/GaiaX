#ifndef D3D_EXTERNAL_RESOURCE_MANAGER_HPP_
#define D3D_EXTERNAL_RESOURCE_MANAGER_HPP_
#include <vector>
#include <ExternalResourceManager.hpp>
#include <D3DExternalResourceFactory.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <StagingBufferManager.hpp>
#include <ReusableVector.hpp>

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

	void UpdateExtensionData(size_t frameIndex) const noexcept;

	void SetGraphicsDescriptorLayout(std::vector<D3DDescriptorManager>& descriptorManagers);

	[[nodiscard]]
	ExternalResourceFactory* GetResourceFactory() noexcept override
	{
		return &m_resourceFactory;
	}

	[[nodiscard]]
	ExternalResourceFactory const* GetResourceFactory() const noexcept override
	{
		return &m_resourceFactory;
	}

private:
	void OnGfxExtensionDeletion(const GraphicsTechniqueExtension& gfxExtension);

	void UpdateDescriptor(
		D3DDescriptorManager& descriptorManager, const ExternalBufferBindingDetails& bindingDetails
	) const;

private:
	D3DExternalResourceFactory     m_resourceFactory;
	ReusableVector<GfxExtension_t> m_gfxExtensions;

	static constexpr size_t s_externalBufferRegisterSpace = 2u;

public:
	D3DExternalResourceManager(const D3DExternalResourceManager&) = delete;
	D3DExternalResourceManager& operator=(const D3DExternalResourceManager&) = delete;

	D3DExternalResourceManager(D3DExternalResourceManager&& other) noexcept
		: m_resourceFactory{ std::move(other.m_resourceFactory) },
		m_gfxExtensions{ std::move(other.m_gfxExtensions) }
	{}
	D3DExternalResourceManager& operator=(D3DExternalResourceManager&& other) noexcept
	{
		m_resourceFactory = std::move(other.m_resourceFactory);
		m_gfxExtensions   = std::move(other.m_gfxExtensions);

		return *this;
	}
};
#endif
