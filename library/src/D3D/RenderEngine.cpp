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
	m_graphicsDescriptorManagers{}, m_graphicsRootSignature{},
	m_textureStorage{ device, &m_memoryManager },
	m_textureManager{},
	m_materialBuffers{ device, &m_memoryManager },
	m_cameraManager{ device, &m_memoryManager },
	m_dsvHeap{ device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
	m_depthBuffer{ device, &m_memoryManager, &m_dsvHeap },
	m_backgroundColour{ 0.0001f, 0.0001f, 0.0001f, 0.0001f },
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

size_t RenderEngine::AddMaterial(std::shared_ptr<Material> material)
{
	WaitForGPUToFinish();

	const size_t index = m_materialBuffers.Add(std::move(material));

	m_materialBuffers.Update(index);

	m_materialBuffers.SetDescriptor(
		m_graphicsDescriptorManagers, s_materialSRVRegisterSlot, s_pixelShaderRegisterSpace
	);

	m_copyNecessary = true;

	return index;
}

std::vector<size_t> RenderEngine::AddMaterials(std::vector<std::shared_ptr<Material>>&& materials)
{
	WaitForGPUToFinish();

	std::vector<size_t> indices = m_materialBuffers.AddMultiple(std::move(materials));

	m_materialBuffers.Update(indices);

	m_materialBuffers.SetDescriptor(
		m_graphicsDescriptorManagers, s_materialSRVRegisterSlot, s_pixelShaderRegisterSpace
	);

	m_copyNecessary = true;

	return indices;
}

void RenderEngine::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept
{
	m_backgroundColour = colourVector;
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
	const UINT globalDescIndex = m_textureStorage.GetTextureBindingIndex(textureIndex);

	if (!std::empty(m_graphicsDescriptorManagers))
		m_textureManager.SetAvailableIndex<D3D12_DESCRIPTOR_RANGE_TYPE_SRV>(globalDescIndex, true);
}

std::uint32_t RenderEngine::BindTexture(size_t textureIndex)
{
	WaitForGPUToFinish();

	static constexpr D3D12_DESCRIPTOR_RANGE_TYPE DescType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	const Texture& texture    = m_textureStorage.Get(textureIndex);
	auto oFreeGlobalDescIndex = m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

	// If there is no free global index, increase the limit. Right now it should be possible to
	// have 65535 bound textures at once. There could be more textures. Adding new descriptors
	// would change the PipelineLayout and that would require every single Pipeline Object to be
	// recreated. Which isn't worth it. And it should be fine to have only 65535 textures bound
	// at once.
	assert(
		oFreeGlobalDescIndex.has_value()
		&& "No descriptor available to bind the texture. Free some already bound textures."
	);

	const UINT freeGlobalDescIndex = oFreeGlobalDescIndex.value();

	m_textureManager.SetAvailableIndex<DescType>(freeGlobalDescIndex, false);
	m_textureStorage.SetTextureBindingIndex(textureIndex, freeGlobalDescIndex);

	for (auto& descriptorManager : m_graphicsDescriptorManagers)
		descriptorManager.CreateSRV(
			s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace, freeGlobalDescIndex,
			texture.Get(), texture.GetSRVDesc()
		);
	// Since it is a descriptor table, there is no point in setting it every time.
	// It should be fine to just bind it once after the descriptorManagers have
	// been created.

	return freeGlobalDescIndex;
}

void RenderEngine::RemoveTexture(size_t index)
{
	WaitForGPUToFinish();

	const UINT globalDescriptorIndex = m_textureStorage.GetTextureBindingIndex(index);
	m_textureManager.SetAvailableIndex<D3D12_DESCRIPTOR_RANGE_TYPE_SRV>(
		globalDescriptorIndex, true
	);

	m_textureStorage.RemoveTexture(index);
}

void RenderEngine::Update(UINT64 frameIndex) const noexcept
{
	m_cameraManager.Update(frameIndex);
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

void RenderEngine::SetCommonGraphicsDescriptorLayout(
	D3D12_SHADER_VISIBILITY cameraShaderVisibility
) noexcept {
	m_cameraManager.SetDescriptorLayoutGraphics(
		m_graphicsDescriptorManagers, GetCameraRegisterSlot(), s_vertexShaderRegisterSpace,
		cameraShaderVisibility
	);
	m_materialBuffers.SetDescriptorLayout(
		m_graphicsDescriptorManagers, s_materialSRVRegisterSlot, s_pixelShaderRegisterSpace
	);
}

std::uint32_t RenderEngine::AddMeshBundle([[maybe_unused]] std::unique_ptr<MeshBundleVS> meshBundle)
{
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddMeshBundle([[maybe_unused]] std::unique_ptr<MeshBundleMS> meshBundle)
{
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddModelBundle(
	[[maybe_unused]] std::shared_ptr<ModelBundleVS>&& modelBundle,
	[[maybe_unused]] const ShaderName& pixelShader
) {
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddModelBundle(
	[[maybe_unused]] std::shared_ptr<ModelBundleMS>&& modelBundle,
	[[maybe_unused]] const ShaderName& pixelShader
) {
	return std::numeric_limits<std::uint32_t>::max();
}
