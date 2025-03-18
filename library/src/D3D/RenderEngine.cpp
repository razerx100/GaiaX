#include <RenderEngine.hpp>

RenderEngine::RenderEngine(
	const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngine{
		deviceManager.GetAdapter(), deviceManager.GetDevice(), std::move(threadPool), frameCount
	}
{}

RenderEngine::RenderEngine(
	IDXGIAdapter3* adapter, ID3D12Device5* device, std::shared_ptr<ThreadPool> threadPool,
	size_t frameCount
) : m_threadPool{ std::move(threadPool) },
	m_memoryManager{ adapter, device, 20_MB, 400_KB },
	// The fences will be initialised as 0. So, the starting value should be 1.
	m_counterValues(frameCount, 0u),
	m_graphicsQueue{}, m_graphicsWait{},
	m_copyQueue{}, m_copyWait{},
	m_stagingManager{ device, &m_memoryManager, m_threadPool.get() },
	m_externalResourceManager{ device, &m_memoryManager },
	m_graphicsDescriptorManagers{}, m_graphicsRootSignature{},
	m_textureStorage{ device, &m_memoryManager },
	m_textureManager{ device },
	m_cameraManager{ device, &m_memoryManager },
	m_dsvHeap{ device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
	m_viewportAndScissors{}, m_temporaryDataBuffer{}, m_copyNecessary{ false }
{
	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_graphicsDescriptorManagers.emplace_back(device, s_graphicsPipelineSetLayoutCount);

		// The graphics Wait semaphores will be used by the Swapchain, which doesn't support
		// timeline semaphores.
		m_graphicsWait.emplace_back().Create(device);
		// Setting the Transfer ones to timeline, as we would want to clean the tempData
		// on a different thread when the transfer queue is finished.
		m_copyWait.emplace_back().Create(device);
	}

	m_graphicsQueue.Create(device, D3D12_COMMAND_LIST_TYPE_DIRECT, frameCount);
	m_copyQueue.Create(device, D3D12_COMMAND_LIST_TYPE_COPY, frameCount);
}

size_t RenderEngine::AddTexture(STexture&& texture)
{
	WaitForGPUToFinish();

	const size_t textureIndex = m_textureStorage.AddTexture(
		std::move(texture), m_stagingManager, m_temporaryDataBuffer
	);

	m_copyNecessary = true;

	return textureIndex;
}

void RenderEngine::UnbindTexture(size_t textureIndex)
{
	// This function shouldn't need to wait for the GPU to finish, as it isn't doing
	// anything on the GPU side.
	static constexpr D3D12_DESCRIPTOR_RANGE_TYPE DescType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	const UINT globalDescIndex = m_textureStorage.GetTextureBindingIndex(textureIndex);

	assert(
		!std::empty(m_graphicsDescriptorManagers)
		&& "The Descriptor Managers should be created before calling this."
	);

	D3DDescriptorManager& descriptorManager = m_graphicsDescriptorManagers.front();

	const D3D12_CPU_DESCRIPTOR_HANDLE globalDescriptor = descriptorManager.GetCPUHandleSRV(
		s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace, globalDescIndex
	);

	const UINT localCacheIndex = m_textureManager.GetFirstFreeLocalDescriptor<DescType>();

	m_textureManager.SetLocalDescriptor<DescType>(globalDescriptor, localCacheIndex);

	m_textureManager.SetLocalDescriptorAvailability<DescType>(localCacheIndex, false);

	m_textureManager.SetBindingAvailability<DescType>(globalDescIndex, true);

	m_textureStorage.SetTextureCacheDetails(static_cast<UINT>(textureIndex), localCacheIndex);
}

UINT RenderEngine::BindTextureCommon(const Texture& texture, std::optional<UINT> oLocalCacheIndex)
{
	WaitForGPUToFinish();

	static constexpr D3D12_DESCRIPTOR_RANGE_TYPE DescType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	std::optional<size_t> oFreeGlobalDescIndex = m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

	// If there is no free global index, increase the limit. Right now it should be possible to
	// have 65535 bound textures at once. There could be more textures.
	if (!oFreeGlobalDescIndex)
	{
		m_textureManager.IncreaseMaximumBindingCount<DescType>();

		for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
		{
			const std::vector<D3DDescriptorLayout> oldLayouts = descriptorManager.GetLayouts();

			m_textureManager.SetDescriptorLayout(
				descriptorManager, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
			);

			descriptorManager.RecreateDescriptors(oldLayouts);
		}

		oFreeGlobalDescIndex = m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

		// Don't need to reset the pipelines, since the table was created as bindless.
	}

	const auto freeGlobalDescIndex = static_cast<UINT>(oFreeGlobalDescIndex.value());

	m_textureManager.SetBindingAvailability<DescType>(freeGlobalDescIndex, false);

	if (oLocalCacheIndex)
	{
		const UINT localCacheIndex = oLocalCacheIndex.value();

		const D3D12_CPU_DESCRIPTOR_HANDLE localDescriptor
			= m_textureManager.GetLocalDescriptor<DescType>(localCacheIndex);

		m_textureManager.SetLocalDescriptorAvailability<DescType>(localCacheIndex, true);

		for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
			descriptorManager.SetDescriptorSRV(
				localDescriptor, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace,
				freeGlobalDescIndex
			);
	}
	else
		for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
			descriptorManager.CreateSRV(
				s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace, freeGlobalDescIndex,
				texture.Get(), texture.GetSRVDesc()
			);
	// Since it is a descriptor table, there is no point in setting it every time.
	// It should be fine to just bind it once after the descriptorManagers have
	// been created.

	return freeGlobalDescIndex;
}

std::uint32_t RenderEngine::BindTexture(size_t textureIndex)
{
	const Texture& texture = m_textureStorage.Get(textureIndex);

	// The current caching system only works for read only single textures which are bound to
	// multiple descriptor managers. Because we only cache one of them.
	std::optional<UINT> localCacheIndex = m_textureStorage.GetAndRemoveTextureLocalDescIndex(
		static_cast<UINT>(textureIndex)
	);

	const UINT freeGlobalDescIndex = BindTextureCommon(texture, localCacheIndex);

	m_textureStorage.SetTextureBindingIndex(textureIndex, freeGlobalDescIndex);

	return freeGlobalDescIndex;
}

void RenderEngine::RemoveTexture(size_t textureIndex)
{
	WaitForGPUToFinish();

	std::vector<UINT> localTextureCacheIndices
		= m_textureStorage.GetAndRemoveTextureCacheDetails(static_cast<UINT>(textureIndex));

	for (UINT localCacheIndex : localTextureCacheIndices)
		m_textureManager.SetLocalDescriptorAvailability<D3D12_DESCRIPTOR_RANGE_TYPE_SRV>(
			localCacheIndex, true
		);

	const UINT globalDescriptorIndex = m_textureStorage.GetTextureBindingIndex(textureIndex);

	m_textureManager.SetBindingAvailability<D3D12_DESCRIPTOR_RANGE_TYPE_SRV>(
		globalDescriptorIndex, true
	);

	m_textureStorage.RemoveTexture(textureIndex);
}

void RenderEngine::WaitForGPUToFinish()
{
	// We will have a counter value per frame. So, we should get which of them
	// has the highest value and signal and wait for that.
	UINT64 highestCounterValue = 0u;

	for (UINT64 counterValue : m_counterValues)
		highestCounterValue = std::max(highestCounterValue, counterValue);

	// Increase the counter value so it gets to a value which hasn't been signalled
	// in the queues yet. So, if we signal this value in the queues now, when it
	// is signalled, we will know that the queues are finished.
	++highestCounterValue;

	for (size_t index = 0u; index < std::size(m_graphicsWait); ++index)
	{
		m_graphicsQueue.Signal(m_graphicsWait[index].Get(), highestCounterValue);
		m_copyQueue.Signal(m_copyWait[index].Get(), highestCounterValue);
	}

	for (size_t index = 0u; index < std::size(m_graphicsWait); ++index)
	{
		m_graphicsWait[index].Wait(highestCounterValue);
		m_copyWait[index].Wait(highestCounterValue);
	}

	for (UINT64& counterValue : m_counterValues)
		counterValue = highestCounterValue;
}

std::vector<std::uint32_t> RenderEngine::AddModelsToBuffer(
	const ModelBundle& modelBundle, ModelBuffers& modelBuffers
) noexcept {
	std::vector<std::shared_ptr<Model>> models = modelBundle.GetModels();

	return modelBuffers.AddMultipleRU32(std::move(models));
}

void RenderEngine::SetCommonGraphicsDescriptorLayout(
	D3D12_SHADER_VISIBILITY cameraShaderVisibility
) noexcept {
	m_cameraManager.SetDescriptorLayoutGraphics(
		m_graphicsDescriptorManagers, GetCameraRegisterSlot(), s_vertexShaderRegisterSpace,
		cameraShaderVisibility
	);
}

void RenderEngine::UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails)
{
	m_externalResourceManager.UpdateDescriptor(m_graphicsDescriptorManagers, bindingDetails);
}

void RenderEngine::UploadExternalBufferGPUOnlyData(
	std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
	size_t dstBufferOffset
) {
	m_externalResourceManager.UploadExternalBufferGPUOnlyData(
		m_stagingManager, m_temporaryDataBuffer,
		externalBufferIndex, std::move(cpuData), srcDataSizeInBytes, dstBufferOffset
	);
}

void RenderEngine::QueueExternalBufferGPUCopy(
	std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
	size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
) {
	m_externalResourceManager.QueueExternalBufferGPUCopy(
		externalBufferSrcIndex, externalBufferDstIndex, dstBufferOffset, srcBufferOffset,
		srcDataSizeInBytes, m_temporaryDataBuffer
	);

	m_copyNecessary = true;
}
