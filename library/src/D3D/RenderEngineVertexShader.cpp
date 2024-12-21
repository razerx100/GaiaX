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

	// RS
	if (!std::empty(m_graphicsDescriptorManagers))
	{
		D3DDescriptorManager& graphicsDescriptorManager = m_graphicsDescriptorManagers.front();

		m_modelManager.SetGraphicsConstantsRootIndex(
			graphicsDescriptorManager, s_vertexShaderRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(graphicsDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::UnboundArray
		);

		m_graphicsRootSignature.CreateSignature(deviceManager.GetDevice(), rootSignatureDynamic);

		m_modelManager.SetGraphicsRootSignature(m_graphicsRootSignature.Get());
	}

	m_cameraManager.CreateBuffer(static_cast<std::uint32_t>(frameCount));

	// This descriptor shouldn't change, so it should be fine to set it here.
	m_cameraManager.SetDescriptorGraphics(
		m_graphicsDescriptorManagers, s_cameraCBVRegisterSlot, s_vertexShaderRegisterSpace
	);
	m_textureManager.SetDescriptorTable(
		m_graphicsDescriptorManagers, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
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
	std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
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

std::uint32_t RenderEngineVSIndividual::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
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

			// Need to copy the old buffers first to avoid empty data being copied over
			// the queued data.
			m_modelManager.CopyOldBuffers(copyCmdListScope);
			m_stagingManager.CopyAndClearQueuedBuffers(copyCmdListScope);
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

		m_graphicsDescriptorManagers[frameIndex].BindDescriptorHeap(graphicsCmdListScope);

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthBuffer.ClearDSV(graphicsCmdListScope);

		renderTarget.Set(graphicsCmdListScope, m_backgroundColour, &dsvHandle);

		m_graphicsRootSignature.BindToGraphics(graphicsCmdListScope);

		m_graphicsDescriptorManagers[frameIndex].BindDescriptors(graphicsCmdListScope);

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
	m_computeQueue{}, m_computeWait{}, m_computeDescriptorManagers{}, m_computeRootSignature{},
	m_commandSignature{}
{
	ID3D12Device5* device = deviceManager.GetDevice();

	// Graphics Descriptors.
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorLayoutVS(
		m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace, s_pixelShaderRegisterSpace
	);
	SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY_VERTEX);

	for (auto& descriptorManager : m_graphicsDescriptorManagers)
		descriptorManager.CreateDescriptors();

	// RS
	if (!std::empty(m_graphicsDescriptorManagers))
	{
		D3DDescriptorManager& graphicsDescriptorManager = m_graphicsDescriptorManagers.front();

		m_modelManager.SetGraphicsConstantsRootIndex(
			graphicsDescriptorManager, s_vertexShaderRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(graphicsDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::UnboundArray
		);

		m_graphicsRootSignature.CreateSignature(device, rootSignatureDynamic);

		m_modelManager.SetGraphicsRootSignature(m_graphicsRootSignature.Get());
	}

	CreateCommandSignature(device);

	m_cameraManager.CreateBuffer(static_cast<std::uint32_t>(frameCount));

	// This descriptor shouldn't change, so it should be fine to set it here.
	m_cameraManager.SetDescriptorGraphics(
		m_graphicsDescriptorManagers, s_cameraCBVRegisterSlot, s_vertexShaderRegisterSpace
	);
	m_textureManager.SetDescriptorTable(
		m_graphicsDescriptorManagers, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
	);

	// Compute stuffs.
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
		m_computeDescriptorManagers, s_cameraCSCBVRegisterSlot, s_computeShaderRegisterSpace
	);

	for (auto& descriptorManagers : m_computeDescriptorManagers)
		descriptorManagers.CreateDescriptors();

	// RS
	if (!std::empty(m_computeDescriptorManagers))
	{
		D3DDescriptorManager& computeDescriptorManager = m_computeDescriptorManagers.front();

		m_modelManager.SetComputeConstantRootIndex(
			computeDescriptorManager, s_computeShaderRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(computeDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.ComputeShader(), BindlessLevel::UnboundArray
		);

		m_computeRootSignature.CreateSignature(device, rootSignatureDynamic);

		m_modelManager.SetComputeRootSignature(m_computeRootSignature.Get());
	}

	m_cameraManager.SetDescriptorCompute(
		m_computeDescriptorManagers, s_cameraCSCBVRegisterSlot, s_computeShaderRegisterSpace
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

void RenderEngineVSIndirect::CreateCommandSignature(ID3D12Device* device)
{
	// Command Signature
	std::array argumentDescs
	{
		D3D12_INDIRECT_ARGUMENT_DESC{
			.Type     = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
			.Constant = {
				.RootParameterIndex      = m_modelManager.GetConstantsVSRootIndex(),
				.DestOffsetIn32BitValues = 0u,
				.Num32BitValuesToSet     = ModelBundleVSIndirect::GetConstantCount()
			}
		},
		D3D12_INDIRECT_ARGUMENT_DESC{ .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED }
	};

	D3D12_COMMAND_SIGNATURE_DESC signatureDesc
	{
		.ByteStride       = static_cast<UINT>(sizeof(ModelBundleCSIndirect::IndirectArgument)),
		.NumArgumentDescs = static_cast<UINT>(std::size(argumentDescs)),
		.pArgumentDescs   = std::data(argumentDescs)
	};

	// If the argumentDesc contains only has the draw type, the rootSignature should be null.
	device->CreateCommandSignature(
		&signatureDesc, m_graphicsRootSignature.Get(), IID_PPV_ARGS(&m_commandSignature)
	);
}

ModelManagerVSIndirect RenderEngineVSIndirect::GetModelManager(
	const DeviceManager& deviceManager, MemoryManager* memoryManager,
	StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
) {
	return ModelManagerVSIndirect{
		deviceManager.GetDevice(), memoryManager, stagingBufferMan, frameCount
	};
}

void RenderEngineVSIndirect::WaitForGPUToFinish()
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
		m_computeQueue.Signal(m_computeWait[index].Get(), highestCounterValue);
		m_copyQueue.Signal(m_copyWait[index].Get(), highestCounterValue);
	}

	for (size_t index = 0u; index < std::size(m_graphicsWait); ++index)
	{
		m_graphicsWait[index].Wait(highestCounterValue);
		m_computeWait[index].Wait(highestCounterValue);
		m_copyWait[index].Wait(highestCounterValue);
	}

	for (UINT64& counterValue : m_counterValues)
		counterValue = highestCounterValue;
}

std::uint32_t RenderEngineVSIndirect::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
) {
	WaitForGPUToFinish();

	const std::uint32_t index = m_modelManager.AddModelBundle(
		std::move(modelBundle), pixelShader, m_temporaryDataBuffer
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

std::uint32_t RenderEngineVSIndirect::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
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

			// Need to copy the old buffers first to avoid empty data being copied over
			// the queued data.
			m_modelManager.CopyOldBuffers(copyCmdListScope);
			m_stagingManager.CopyAndClearQueuedBuffers(copyCmdListScope);
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

		m_computeDescriptorManagers[frameIndex].BindDescriptorHeap(computeCmdListScope);

		m_computeRootSignature.BindToCompute(computeCmdListScope);

		m_computeDescriptorManagers[frameIndex].BindDescriptors(computeCmdListScope);

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

		m_graphicsDescriptorManagers[frameIndex].BindDescriptorHeap(graphicsCmdListScope);

		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthBuffer.ClearDSV(graphicsCmdListScope);

		renderTarget.Set(graphicsCmdListScope, m_backgroundColour, &dsvHandle);

		m_graphicsRootSignature.BindToGraphics(graphicsCmdListScope);

		m_graphicsDescriptorManagers[frameIndex].BindDescriptors(graphicsCmdListScope);

		m_modelManager.Draw(frameIndex, graphicsCmdListScope, m_commandSignature.Get());

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
