#include <D3DRenderEngine.hpp>

namespace Gaia
{
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
	m_memoryManager{ std::make_unique<MemoryManager>(adapter, device, 20_MB, 400_KB) },
	// The fences will be initialised as 0. So, the starting value should be 1.
	m_counterValues(frameCount, 0u),
	m_graphicsQueue{}, m_graphicsWait{},
	m_copyQueue{}, m_copyWait{},
	m_stagingManager{ device, m_memoryManager.get(), m_threadPool.get()},
	m_dsvHeap{
		std::make_unique<D3DReusableDescriptorHeap>(
			device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		)
	},
	m_graphicsDescriptorManagers{},
	m_externalResourceManager{
		std::make_unique<D3DExternalResourceManager>(device, m_memoryManager.get())
	},
	m_graphicsRootSignature{},
	m_textureStorage{ device, m_memoryManager.get() },
	m_textureManager{ device },
	m_cameraManager{ device, m_memoryManager.get() },
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

void RenderEngine::UnbindTexture(size_t textureIndex, UINT bindingIndex)
{
	// This function shouldn't need to wait for the GPU to finish, as it isn't doing
	// anything on the GPU side.
	static constexpr D3D12_DESCRIPTOR_RANGE_TYPE DescType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	assert(
		!std::empty(m_graphicsDescriptorManagers)
		&& "The Descriptor Managers should be created before calling this."
	);

	D3DDescriptorManager& descriptorManager = m_graphicsDescriptorManagers.front();

	const D3D12_CPU_DESCRIPTOR_HANDLE globalDescriptor = descriptorManager.GetCPUHandleSRV(
		s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace, bindingIndex
	);

	const UINT localCacheIndex = m_textureManager.GetFirstFreeLocalDescriptor<DescType>();

	m_textureManager.SetLocalDescriptor<DescType>(globalDescriptor, localCacheIndex);

	m_textureManager.SetLocalDescriptorAvailability<DescType>(localCacheIndex, false);

	m_textureManager.SetBindingAvailability<DescType>(bindingIndex, true);

	m_textureStorage.SetTextureCacheDetails(static_cast<UINT>(textureIndex), localCacheIndex);
}

void RenderEngine::UnbindExternalTexture(UINT bindingIndex)
{
	static constexpr D3D12_DESCRIPTOR_RANGE_TYPE DescType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	m_textureManager.SetBindingAvailability<DescType>(bindingIndex, true);
}

void RenderEngine::RebindExternalTexture(size_t textureIndex, UINT bindingIndex)
{
	D3DExternalResourceFactory* resourceFactory
		= m_externalResourceManager->GetD3DResourceFactory();

	const Texture& texture = resourceFactory->GetD3DTexture(textureIndex);

	for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
		descriptorManager.CreateSRV(
			s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace, bindingIndex,
			texture.Get(), texture.GetSRVDesc()
		);
}

void RenderEngine::WaitForGraphicsQueueToFinish()
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

void RenderEngine::UpdateExternalBufferDescriptor(
	const ExternalBufferBindingDetails& bindingDetails
) {
	m_externalResourceManager->UpdateDescriptor(m_graphicsDescriptorManagers, bindingDetails);
}

void RenderEngine::UploadExternalBufferGPUOnlyData(
	std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
	size_t dstBufferOffset
) {
	m_externalResourceManager->UploadExternalBufferGPUOnlyData(
		m_stagingManager, m_temporaryDataBuffer,
		externalBufferIndex, std::move(cpuData), srcDataSizeInBytes, dstBufferOffset
	);
}

void RenderEngine::QueueExternalBufferGPUCopy(
	std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
	size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
) {
	m_externalResourceManager->QueueExternalBufferGPUCopy(
		externalBufferSrcIndex, externalBufferDstIndex, dstBufferOffset, srcBufferOffset,
		srcDataSizeInBytes, m_temporaryDataBuffer
	);

	m_copyNecessary = true;
}
}
