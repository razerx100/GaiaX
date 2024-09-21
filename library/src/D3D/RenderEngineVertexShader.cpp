#include <RenderEngineVertexShader.hpp>

// VS Individual
RenderEngineVSIndividual::RenderEngineVSIndividual(
	const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorLayout(
		m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace, s_pixelShaderRegisterSpace
	);
	SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY_VERTEX);

	for (auto& descriptorManager : m_graphicsDescriptorManagers)
		descriptorManager.CreateDescriptors();

	if (!std::empty(m_graphicsDescriptorManagers))
		m_modelManager.CreateRootSignature(
			m_graphicsDescriptorManagers.front(), s_vertexShaderRegisterSpace
		);

	m_cameraManager.CreateBuffer(static_cast<std::uint32_t>(frameCount));

	// This descriptor shouldn't change, so it should be fine to set it here.
	m_cameraManager.SetDescriptorGraphics(
		m_graphicsDescriptorManagers, s_cameraRegisterSlot, s_vertexShaderRegisterSpace
	);
	m_textureManager.SetDescriptorTable(
		m_graphicsDescriptorManagers, s_textureRegisterSlot, s_pixelShaderRegisterSpace
	);

	SetupPipelineStages();
}

void RenderEngineVSIndividual::SetupPipelineStages()
{
	constexpr size_t stageCount = 2u;

	m_pipelineStages.reserve(stageCount);

	m_pipelineStages.emplace_back(&RenderEngineVSIndividual::GenericCopyStage);
	m_pipelineStages.emplace_back(&RenderEngineVSIndividual::DrawingStage);
}

ModelManagerVSIndividual RenderEngineVSIndividual::GetModelManager(
	const DeviceManager& deviceManager, MemoryManager* memoryManager,
	[[maybe_unused]] StagingBufferManager* stagingBufferMan,
	std::uint32_t frameCount
) {
	return ModelManagerVSIndividual{ deviceManager.GetDevice(), memoryManager, frameCount };
}

std::uint32_t RenderEngineVSIndividual::AddModelBundle(
	std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& pixelShader
) {
	WaitForGPUToFinish();

	const std::uint32_t index = m_modelManager.AddModelBundle(
		std::move(modelBundle), pixelShader, m_temporaryDataBuffer
	);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptors(
		m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace, s_pixelShaderRegisterSpace
	);

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineVSIndividual::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	WaitForGPUToFinish();

	m_copyNecessary = true;

	return m_modelManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);
}

ID3D12Fence* RenderEngineVSIndividual::GenericCopyStage(
	size_t frameIndex,
	[[maybe_unused]] const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
) {
	// Copy Phase
	// If the Copy stage isn't executed, pass the waitFence on.
	ID3D12Fence* signalledFence = waitFence;

	if (m_copyNecessary)
	{
		const D3DCommandList& copyCmdList = m_copyQueue.GetCommandList(frameIndex);

		{
			const CommandListScope copyCmdListScope{ copyCmdList };

			m_stagingManager.CopyAndClear(copyCmdListScope);
			m_modelManager.CopyTempData(copyCmdListScope);
		}

		const D3DFence& copyWaitFence = m_copyWait[frameIndex];

		{
			const UINT64 oldCounterValue = counterValue;
			++counterValue;

			QueueSubmitBuilder<1u, 1u> copySubmitBuilder{};
			copySubmitBuilder
				.SignalFence(copyWaitFence, counterValue)
				.WaitFence(waitFence, oldCounterValue)
				.CommandList(copyCmdList);

			m_copyQueue.SubmitCommandLists(copySubmitBuilder);

			m_temporaryDataBuffer.SetUsed(frameIndex);
		}

		m_copyNecessary = false;
		signalledFence  = copyWaitFence.Get();
	}

	return signalledFence;
}

ID3D12Fence* RenderEngineVSIndividual::DrawingStage(
	size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
) {
	// Graphics Phase
	const D3DCommandList& graphicsCmdList = m_graphicsQueue.GetCommandList(frameIndex);

	{
		const CommandListScope graphicsCmdListScope{ graphicsCmdList };

		renderTarget.ToRenderState(graphicsCmdListScope);

		m_textureStorage.TransitionQueuedTextures(graphicsCmdListScope);

		m_viewportAndScissors.Bind(graphicsCmdListScope);

		m_graphicsDescriptorManagers[frameIndex].Bind(graphicsCmdListScope);

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthBuffer.ClearDSV(graphicsCmdListScope);

		renderTarget.Set(graphicsCmdListScope, m_backgroundColour, &dsvHandle);

		m_modelManager.Draw(graphicsCmdListScope);

		renderTarget.ToPresentState(graphicsCmdListScope);
	}

	const D3DFence& graphicsWaitFence = m_graphicsWait[frameIndex];

	{
		const UINT64 oldCounterValue = counterValue;
		++counterValue;

		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalFence(graphicsWaitFence, counterValue)
			.WaitFence(waitFence, oldCounterValue)
			.CommandList(graphicsCmdList);

		m_graphicsQueue.SubmitCommandLists(graphicsSubmitBuilder);
	}

	return graphicsWaitFence.Get();
}

// VS Indirect
RenderEngineVSIndirect::RenderEngineVSIndirect(
	const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount },
	m_computeQueue{}, m_computeWait{}, m_computeDescriptorManagers{}
{
	// Graphics Descriptors.
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorLayoutVS(
		m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace, s_pixelShaderRegisterSpace
	);
	SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY_VERTEX);

	for (auto& descriptorManager : m_graphicsDescriptorManagers)
		descriptorManager.CreateDescriptors();

	if (!std::empty(m_graphicsDescriptorManagers))
		m_modelManager.CreateRootSignature(
			m_graphicsDescriptorManagers.front(), s_vertexShaderRegisterSpace
		);

	m_cameraManager.CreateBuffer(static_cast<std::uint32_t>(frameCount));

	// This descriptor shouldn't change, so it should be fine to set it here.
	m_cameraManager.SetDescriptorGraphics(
		m_graphicsDescriptorManagers, s_cameraRegisterSlot, s_vertexShaderRegisterSpace
	);
	m_textureManager.SetDescriptorTable(
		m_graphicsDescriptorManagers, s_textureRegisterSlot, s_pixelShaderRegisterSpace
	);

	// Compute stuffs.
	ID3D12Device5* device = deviceManager.GetDevice();

	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_computeDescriptorManagers.emplace_back(device, s_computePipelineSetLayoutCount);

		// Let's make all of the non graphics fences.
		m_computeWait.emplace_back().Create(device);
	}

	const auto frameCountU32 = static_cast<std::uint32_t>(frameCount);

	m_computeQueue.Create(device, D3D12_COMMAND_LIST_TYPE_COMPUTE, frameCountU32);

	// Compute Descriptors.
	m_modelManager.SetDescriptorLayoutCS(m_computeDescriptorManagers, s_computeShaderRegisterSpace);
	m_cameraManager.SetDescriptorLayoutCompute(
		m_computeDescriptorManagers, s_cameraComputeRegisterSlot, s_computeShaderRegisterSpace
	);

	for (auto& descriptorManagers : m_computeDescriptorManagers)
		descriptorManagers.CreateDescriptors();

	if (!std::empty(m_computeDescriptorManagers))
		m_modelManager.CreatePipelineCS(m_computeDescriptorManagers.front(), s_computeShaderRegisterSpace);

	m_cameraManager.SetDescriptorCompute(
		m_computeDescriptorManagers, s_cameraComputeRegisterSlot, s_computeShaderRegisterSpace
	);

	SetupPipelineStages();
}

void RenderEngineVSIndirect::SetupPipelineStages()
{
	constexpr size_t stageCount = 3u;

	m_pipelineStages.reserve(stageCount);

	m_pipelineStages.emplace_back(&RenderEngineVSIndirect::GenericCopyStage);
	m_pipelineStages.emplace_back(&RenderEngineVSIndirect::FrustumCullingStage);
	m_pipelineStages.emplace_back(&RenderEngineVSIndirect::DrawingStage);
}

ModelManagerVSIndirect RenderEngineVSIndirect::GetModelManager(
	const DeviceManager& deviceManager, MemoryManager* memoryManager,
	StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
) {
	return ModelManagerVSIndirect{
		deviceManager.GetDevice(), memoryManager, stagingBufferMan, frameCount
	};
}

std::uint32_t RenderEngineVSIndirect::AddModelBundle(
	std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& fragmentShader
) {
	WaitForGPUToFinish();

	const std::uint32_t index = m_modelManager.AddModelBundle(
		std::move(modelBundle), fragmentShader, m_temporaryDataBuffer
	);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorsVS(
		m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace, s_pixelShaderRegisterSpace
	);
	m_modelManager.SetDescriptorsCSOfModels(m_computeDescriptorManagers, s_computeShaderRegisterSpace);

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineVSIndirect::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	WaitForGPUToFinish();

	const std::uint32_t index = m_modelManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);

	m_modelManager.SetDescriptorsCSOfMeshes(m_computeDescriptorManagers, s_computeShaderRegisterSpace);

	m_copyNecessary = true;

	return index;
}

ID3D12Fence* RenderEngineVSIndirect::GenericCopyStage(
	size_t frameIndex,
	[[maybe_unused]] const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
) {
	// Copy Phase
	// If the Copy stage isn't executed, pass the waitFence on.
	ID3D12Fence* signalledFence = waitFence;

	if (m_copyNecessary)
	{
		const D3DCommandList& copyCmdList = m_copyQueue.GetCommandList(frameIndex);

		{
			const CommandListScope copyCmdListScope{ copyCmdList };

			m_stagingManager.CopyAndClear(copyCmdListScope);
			m_modelManager.CopyTempBuffers(copyCmdListScope);
		}

		const D3DFence& copyWaitFence = m_copyWait[frameIndex];

		{
			const UINT64 oldCounterValue = counterValue;
			++counterValue;

			QueueSubmitBuilder<1u, 1u> copySubmitBuilder{};
			copySubmitBuilder
				.SignalFence(copyWaitFence, counterValue)
				.WaitFence(waitFence, oldCounterValue)
				.CommandList(copyCmdList);

			m_copyQueue.SubmitCommandLists(copySubmitBuilder);

			m_temporaryDataBuffer.SetUsed(frameIndex);
		}

		m_copyNecessary = false;
		signalledFence  = copyWaitFence.Get();
	}

	return signalledFence;
}

ID3D12Fence* RenderEngineVSIndirect::FrustumCullingStage(
	size_t frameIndex,
	[[maybe_unused]] const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
) {
	// Compute Phase
	const D3DCommandList& computeCmdList = m_computeQueue.GetCommandList(frameIndex);

	{
		const CommandListScope computeCmdListScope{ computeCmdList };

		m_modelManager.ResetCounterBuffer(computeCmdList, static_cast<UINT64>(frameIndex));

		m_computeDescriptorManagers[frameIndex].Bind(computeCmdListScope);

		m_modelManager.Dispatch(computeCmdListScope);
	}

	const D3DFence& computeWaitFence = m_computeWait[frameIndex];

	{
		const UINT64 oldCounterValue = counterValue;
		++counterValue;

		QueueSubmitBuilder<1u, 1u> computeSubmitBuilder{};
		computeSubmitBuilder
			.SignalFence(computeWaitFence, counterValue)
			.WaitFence(waitFence, oldCounterValue)
			.CommandList(computeCmdList);

		m_computeQueue.SubmitCommandLists(computeSubmitBuilder);
	}

	return computeWaitFence.Get();
}

ID3D12Fence* RenderEngineVSIndirect::DrawingStage(
	size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
) {
	// Graphics Phase
	const D3DCommandList& graphicsCmdList = m_graphicsQueue.GetCommandList(frameIndex);

	{
		const CommandListScope graphicsCmdListScope{ graphicsCmdList };

		renderTarget.ToRenderState(graphicsCmdListScope);

		m_textureStorage.TransitionQueuedTextures(graphicsCmdListScope);

		m_viewportAndScissors.Bind(graphicsCmdListScope);

		m_graphicsDescriptorManagers[frameIndex].Bind(graphicsCmdListScope);

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthBuffer.ClearDSV(graphicsCmdListScope);

		renderTarget.Set(graphicsCmdListScope, m_backgroundColour, &dsvHandle);

		m_modelManager.Draw(frameIndex, graphicsCmdListScope);

		renderTarget.ToPresentState(graphicsCmdListScope);
	}

	const D3DFence& graphicsWaitFence = m_graphicsWait[frameIndex];

	{
		const UINT64 oldCounterValue = counterValue;
		++counterValue;

		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalFence(graphicsWaitFence, counterValue)
			.WaitFence(waitFence, oldCounterValue)
			.CommandList(graphicsCmdList);

		m_graphicsQueue.SubmitCommandLists(graphicsSubmitBuilder);
	}

	return graphicsWaitFence.Get();
}
