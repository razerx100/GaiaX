#include <RenderEngineVS.hpp>

// VS Individual
RenderEngineVSIndividual::RenderEngineVSIndividual(
	const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	m_modelManager = std::make_unique<ModelManagerVSIndividual>();

	SetGraphicsDescriptorBufferLayout();

	m_cameraManager.CreateBuffer(static_cast<std::uint32_t>(frameCount));
}

void RenderEngineVSIndividual::FinaliseInitialisation(const DeviceManager& deviceManager)
{
	m_externalResourceManager->SetGraphicsDescriptorLayout(m_graphicsDescriptorManagers);

	for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
		descriptorManager.CreateDescriptors();

	// RS
	if (!std::empty(m_graphicsDescriptorManagers))
	{
		D3DDescriptorManager& graphicsDescriptorManager = m_graphicsDescriptorManagers.front();

		m_modelManager->SetGraphicsConstantsRootIndex(
			graphicsDescriptorManager, s_vertexShaderRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(graphicsDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::UnboundArray
		);

		m_graphicsRootSignature.CreateSignature(deviceManager.GetDevice(), rootSignatureDynamic);

		m_graphicsPipelineManager.SetRootSignature(m_graphicsRootSignature.Get());
	}

	m_cameraManager.SetDescriptorGraphics(
		m_graphicsDescriptorManagers, s_cameraCBVRegisterSlot, s_vertexShaderRegisterSpace
	);
	m_textureManager.SetDescriptorTable(
		m_graphicsDescriptorManagers, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
	);
}

void RenderEngineVSIndividual::SetGraphicsDescriptorBufferLayout()
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager->SetDescriptorLayout(m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace);
	SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY_ALL);

	for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
	{
		descriptorManager.AddRootSRV(
			s_modelBuffersGraphicsSRVRegisterSlot, s_vertexShaderRegisterSpace,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersPixelSRVRegisterSlot, s_pixelShaderRegisterSpace,
			D3D12_SHADER_VISIBILITY_PIXEL
		);
	}
}

void RenderEngineVSIndividual::SetGraphicsDescriptors()
{
	const size_t frameCount = std::size(m_graphicsDescriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = m_graphicsDescriptorManagers[index];
		const auto frameIndex                   = static_cast<UINT64>(index);

		m_modelBuffers.SetDescriptor(
			descriptorManager, frameIndex, s_modelBuffersGraphicsSRVRegisterSlot,
			s_vertexShaderRegisterSpace, true
		);
		m_modelBuffers.SetPixelDescriptor(
			descriptorManager, frameIndex, s_modelBuffersPixelSRVRegisterSlot,
			s_pixelShaderRegisterSpace
		);
	}
}

void RenderEngineVSIndividual::ExecutePipelineStages(
	size_t frameIndex, ID3D12Resource* swapchainBackBuffer, UINT64& counterValue,
	ID3D12Fence* waitFence
) {
	waitFence = GenericCopyStage(frameIndex, counterValue, waitFence);

	DrawingStage(frameIndex, swapchainBackBuffer, counterValue, waitFence);
}

std::uint32_t RenderEngineVSIndividual::AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle)
{
	WaitForGPUToFinish();

	std::vector<std::uint32_t> modelBufferIndices = AddModelsToBuffer(*modelBundle, m_modelBuffers);

	const std::uint32_t index = m_modelManager->AddModelBundle(
		std::move(modelBundle), modelBufferIndices
	);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	SetGraphicsDescriptors();

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineVSIndividual::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
{
	WaitForGPUToFinish();

	m_copyNecessary = true;

	return m_meshManager.AddMeshBundle(std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer);
}

ID3D12Fence* RenderEngineVSIndividual::GenericCopyStage(
	size_t frameIndex, UINT64& counterValue, ID3D12Fence* waitFence
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
			m_externalResourceManager->CopyQueuedBuffers(copyCmdListScope);
			m_meshManager.CopyOldBuffers(copyCmdListScope);
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

void RenderEngineVSIndividual::DrawRenderPassPipelines(
	const D3DCommandList& graphicsCmdList, const ExternalRenderPass_t& renderPass
) const noexcept {
	const std::vector<D3DExternalRenderPass::PipelineDetails>& pipelineDetails
		= renderPass.GetPipelineDetails();

	for (const D3DExternalRenderPass::PipelineDetails& details : pipelineDetails)
	{
		const std::vector<std::uint32_t>& bundleIndices        = details.modelBundleIndices;
		const std::vector<std::uint32_t>& pipelineLocalIndices = details.pipelineLocalIndices;

		m_graphicsPipelineManager.BindPipeline(details.pipelineGlobalIndex, graphicsCmdList);

		const size_t bundleCount = std::size(bundleIndices);

		for (size_t index = 0u; index < bundleCount; ++index)
			m_modelManager->DrawPipeline(
				bundleIndices[index], pipelineLocalIndices[index], graphicsCmdList, m_meshManager
			);
	}
}

void RenderEngineVSIndividual::DrawingStage(
	size_t frameIndex, ID3D12Resource* swapchainBackBuffer, UINT64& counterValue, ID3D12Fence* waitFence
) {
	// Graphics Phase
	const D3DCommandList& graphicsCmdList = m_graphicsQueue.GetCommandList(frameIndex);

	{
		const CommandListScope graphicsCmdListScope{ graphicsCmdList };

		m_textureStorage.TransitionQueuedTextures(graphicsCmdListScope);

		m_viewportAndScissors.Bind(graphicsCmdListScope);

		m_graphicsDescriptorManagers[frameIndex].BindDescriptorHeap(graphicsCmdListScope);

		m_graphicsRootSignature.BindToGraphics(graphicsCmdListScope);

		m_graphicsDescriptorManagers[frameIndex].BindDescriptors(graphicsCmdListScope);

		// Normal passes
		const size_t renderPassCount = std::size(m_renderPasses);

		for (size_t index = 0u; index < renderPassCount; ++index)
		{
			if (!m_renderPasses.IsInUse(index))
				continue;

			const ExternalRenderPass_t& renderPass = *m_renderPasses[index];

			renderPass.StartPass(graphicsCmdListScope);

			DrawRenderPassPipelines(graphicsCmdListScope, renderPass);

			// No need to end passes for non swapchain passes.
		}

		// The one for the swapchain
		if (m_swapchainRenderPass)
		{
			const ExternalRenderPass_t& renderPass = *m_swapchainRenderPass;

			renderPass.StartPass(graphicsCmdListScope);

			DrawRenderPassPipelines(graphicsCmdListScope, renderPass);

			renderPass.EndPassForSwapchain(graphicsCmdListScope, swapchainBackBuffer);
		}
	}

	{
		const D3DFence& graphicsWaitFence = m_graphicsWait[frameIndex];

		const UINT64 oldCounterValue = counterValue;
		++counterValue;

		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalFence(graphicsWaitFence, counterValue)
			.WaitFence(waitFence, oldCounterValue)
			.CommandList(graphicsCmdList);

		m_graphicsQueue.SubmitCommandLists(graphicsSubmitBuilder);
	}
}

// VS Indirect
RenderEngineVSIndirect::RenderEngineVSIndirect(
	const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount },
	m_computeQueue{}, m_computeWait{}, m_computeDescriptorManagers{},
	m_computePipelineManager{ deviceManager.GetDevice() }, m_computeRootSignature{}, m_commandSignature{}
{
	m_modelManager = std::make_unique<ModelManagerVSIndirect>(
		deviceManager.GetDevice(), m_memoryManager.get(), static_cast<std::uint32_t>(frameCount)
	);

	ID3D12Device5* device = deviceManager.GetDevice();

	SetGraphicsDescriptorBufferLayout();

	m_cameraManager.CreateBuffer(static_cast<std::uint32_t>(frameCount));

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
	SetComputeDescriptorBufferLayout();
}

void RenderEngineVSIndirect::FinaliseInitialisation(const DeviceManager& deviceManager)
{
	ID3D12Device5* device = deviceManager.GetDevice();

	m_externalResourceManager->SetGraphicsDescriptorLayout(m_graphicsDescriptorManagers);

	// Graphics
	for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
		descriptorManager.CreateDescriptors();

	// RS
	if (!std::empty(m_graphicsDescriptorManagers))
	{
		D3DDescriptorManager& graphicsDescriptorManager = m_graphicsDescriptorManagers.front();

		m_modelManager->SetGraphicsConstantsRootIndex(
			graphicsDescriptorManager, s_vertexShaderRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(graphicsDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::UnboundArray
		);

		m_graphicsRootSignature.CreateSignature(device, rootSignatureDynamic);

		m_graphicsPipelineManager.SetRootSignature(m_graphicsRootSignature.Get());
	}

	CreateCommandSignature(device);

	m_cameraManager.SetDescriptorGraphics(
		m_graphicsDescriptorManagers, s_cameraCBVRegisterSlot, s_vertexShaderRegisterSpace
	);
	m_textureManager.SetDescriptorTable(
		m_graphicsDescriptorManagers, s_textureSRVRegisterSlot, s_pixelShaderRegisterSpace
	);

	// Compute
	for (D3DDescriptorManager& descriptorManagers : m_computeDescriptorManagers)
		descriptorManagers.CreateDescriptors();

	// RS
	if (!std::empty(m_computeDescriptorManagers))
	{
		D3DDescriptorManager& computeDescriptorManager = m_computeDescriptorManagers.front();

		m_modelManager->SetComputeConstantsRootIndex(
			computeDescriptorManager, s_computeShaderRegisterSpace
		);

		D3DRootSignatureDynamic rootSignatureDynamic{};

		rootSignatureDynamic.PopulateFromLayouts(computeDescriptorManager.GetLayouts());

		rootSignatureDynamic.CompileSignature(
			RSCompileFlagBuilder{}.ComputeShader(), BindlessLevel::UnboundArray
		);

		m_computeRootSignature.CreateSignature(device, rootSignatureDynamic);

		m_computePipelineManager.SetRootSignature(m_computeRootSignature.Get());
	}

	m_cameraManager.SetDescriptorCompute(
		m_computeDescriptorManagers, s_cameraCSCBVRegisterSlot, s_computeShaderRegisterSpace
	);

	assert(
		!std::empty(m_computePipelineManager.GetShaderPath())
		&& "The shader path should be set before calling this function."
	);

	// Add the Frustum Culling shader.
	const std::uint32_t frustumCSOIndex = m_computePipelineManager.AddOrGetComputePipeline(
		ShaderName{ L"VertexShaderCSIndirect" }
	);

	m_modelManager->SetCSPSOIndex(frustumCSOIndex);
}

void RenderEngineVSIndirect::SetGraphicsDescriptorBufferLayout()
{
	// Graphics Descriptors.
	// The layout shouldn't change throughout the runtime.
	m_modelManager->SetDescriptorLayoutVS(m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace);
	SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY_ALL);

	const auto frameCount = std::size(m_graphicsDescriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = m_graphicsDescriptorManagers[index];

		descriptorManager.AddRootSRV(
			s_modelBuffersGraphicsSRVRegisterSlot, s_vertexShaderRegisterSpace,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersPixelSRVRegisterSlot, s_pixelShaderRegisterSpace,
			D3D12_SHADER_VISIBILITY_PIXEL
		);
	}
}

void RenderEngineVSIndirect::SetComputeDescriptorBufferLayout()
{
	m_modelManager->SetDescriptorLayoutCS(m_computeDescriptorManagers, s_computeShaderRegisterSpace);
	m_meshManager.SetDescriptorLayoutCS(m_computeDescriptorManagers, s_computeShaderRegisterSpace);
	m_cameraManager.SetDescriptorLayoutCompute(
		m_computeDescriptorManagers, s_cameraCSCBVRegisterSlot, s_computeShaderRegisterSpace
	);

	for (D3DDescriptorManager& descriptorManager : m_computeDescriptorManagers)
		descriptorManager.AddRootSRV(
			s_modelBuffersCSSRVRegisterSlot, s_computeShaderRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
}

void RenderEngineVSIndirect::ExecutePipelineStages(
	size_t frameIndex, ID3D12Resource* swapchainBackBuffer, UINT64& counterValue,
	ID3D12Fence* waitFence
) {
	waitFence = GenericCopyStage(frameIndex, counterValue, waitFence);

	waitFence = FrustumCullingStage(frameIndex, counterValue, waitFence);

	DrawingStage(frameIndex, swapchainBackBuffer, counterValue, waitFence);
}

void RenderEngineVSIndirect::SetShaderPath(const std::wstring& shaderPath)
{
	RenderEngineCommon::SetShaderPath(shaderPath);

	m_computePipelineManager.SetShaderPath(shaderPath);
}

void RenderEngineVSIndirect::UpdateRenderPassPipelines(
	size_t frameIndex, const ExternalRenderPass_t& renderPass
) const noexcept {
	const std::vector<D3DExternalRenderPass::PipelineDetails>& pipelineDetails
		= renderPass.GetPipelineDetails();

	for (const D3DExternalRenderPass::PipelineDetails& details : pipelineDetails)
	{
		const std::vector<std::uint32_t>& bundleIndices        = details.modelBundleIndices;
		const std::vector<std::uint32_t>& pipelineLocalIndices = details.pipelineLocalIndices;

		const GraphicsPipelineVSIndirectDraw& d3dPipeline
			= m_graphicsPipelineManager.GetPipeline(details.pipelineGlobalIndex);

		const size_t bundleCount = std::size(bundleIndices);

		for (size_t index = 0u; index < bundleCount; ++index)
			m_modelManager->UpdatePipelinePerFrame(
				frameIndex, bundleIndices[index], pipelineLocalIndices[index], m_meshManager,
				!d3dPipeline.GetExternalPipeline().IsGPUCullingEnabled()
			);
	}
}

void RenderEngineVSIndirect::_updatePerFrame(UINT64 frameIndex) const noexcept
{
	m_modelBuffers.Update(frameIndex);

		// Normal passes
	const size_t renderPassCount = std::size(m_renderPasses);

	for (size_t index = 0u; index < renderPassCount; ++index)
	{
		if (!m_renderPasses.IsInUse(index))
			continue;

		UpdateRenderPassPipelines(frameIndex, *m_renderPasses[index]);
	}

	// The one for the swapchain
	if (m_swapchainRenderPass)
		UpdateRenderPassPipelines(frameIndex, *m_swapchainRenderPass);
}

void RenderEngineVSIndirect::CreateCommandSignature(ID3D12Device* device)
{
	// Command Signature
	std::array argumentDescs
	{
		D3D12_INDIRECT_ARGUMENT_DESC{
			.Type     = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
			.Constant = {
				.RootParameterIndex      = m_modelManager->GetConstantsVSRootIndex(),
				.DestOffsetIn32BitValues = 0u,
				.Num32BitValuesToSet     = PipelineModelsVSIndirect::GetConstantCount()
			}
		},
		D3D12_INDIRECT_ARGUMENT_DESC{ .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED }
	};

	D3D12_COMMAND_SIGNATURE_DESC signatureDesc
	{
		.ByteStride       = static_cast<UINT>(sizeof(PipelineModelsCSIndirect::IndirectArgument)),
		.NumArgumentDescs = static_cast<UINT>(std::size(argumentDescs)),
		.pArgumentDescs   = std::data(argumentDescs)
	};

	// If the argumentDesc contains only has the draw type, the rootSignature should be null.
	device->CreateCommandSignature(
		&signatureDesc, m_graphicsRootSignature.Get(), IID_PPV_ARGS(&m_commandSignature)
	);
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


void RenderEngineVSIndirect::SetModelGraphicsDescriptors()
{
	const size_t frameCount = std::size(m_graphicsDescriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = m_graphicsDescriptorManagers[index];
		const auto frameIndex                   = static_cast<UINT64>(index);

		m_modelBuffers.SetDescriptor(
			descriptorManager, frameIndex, s_modelBuffersGraphicsSRVRegisterSlot,
			s_vertexShaderRegisterSpace, true
		);
		m_modelBuffers.SetPixelDescriptor(
			descriptorManager, frameIndex, s_modelBuffersPixelSRVRegisterSlot,
			s_pixelShaderRegisterSpace
		);
	}
}

void RenderEngineVSIndirect::SetModelComputeDescriptors()
{
	m_modelManager->SetDescriptors(m_computeDescriptorManagers, s_computeShaderRegisterSpace);

	const size_t frameCount = std::size(m_computeDescriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = m_computeDescriptorManagers[index];
		const auto frameIndex                   = static_cast<UINT64>(index);

		m_modelBuffers.SetDescriptor(
			descriptorManager, frameIndex, s_modelBuffersCSSRVRegisterSlot,
			s_computeShaderRegisterSpace, false
		);
	}
}

std::uint32_t RenderEngineVSIndirect::AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle)
{
	WaitForGPUToFinish();

	std::vector<std::uint32_t> modelBufferIndices = AddModelsToBuffer(*modelBundle, m_modelBuffers);

	const std::uint32_t index = m_modelManager->AddModelBundle(
		std::move(modelBundle), modelBufferIndices
	);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	SetModelGraphicsDescriptors();
	SetModelComputeDescriptors();

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineVSIndirect::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
{
	WaitForGPUToFinish();

	const std::uint32_t index = m_meshManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);

	m_meshManager.SetDescriptorsCS(m_computeDescriptorManagers, s_computeShaderRegisterSpace);

	m_copyNecessary = true;

	return index;
}

ID3D12Fence* RenderEngineVSIndirect::GenericCopyStage(
	size_t frameIndex, UINT64& counterValue, ID3D12Fence* waitFence
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
			m_externalResourceManager->CopyQueuedBuffers(copyCmdListScope);
			m_meshManager.CopyOldBuffers(copyCmdListScope);
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
	size_t frameIndex, UINT64& counterValue, ID3D12Fence* waitFence
) {
	// Compute Phase
	const D3DCommandList& computeCmdList = m_computeQueue.GetCommandList(frameIndex);

	{
		const CommandListScope computeCmdListScope{ computeCmdList };

		m_modelManager->ResetCounterBuffer(computeCmdList, static_cast<UINT64>(frameIndex));

		m_computeDescriptorManagers[frameIndex].BindDescriptorHeap(computeCmdListScope);

		m_computeRootSignature.BindToCompute(computeCmdListScope);

		m_computeDescriptorManagers[frameIndex].BindDescriptors(computeCmdListScope);

		m_modelManager->Dispatch(computeCmdListScope, m_computePipelineManager);
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

void RenderEngineVSIndirect::DrawRenderPassPipelines(
	size_t frameIndex, const D3DCommandList& graphicsCmdList, const ExternalRenderPass_t& renderPass
) const noexcept {
	const std::vector<D3DExternalRenderPass::PipelineDetails>& pipelineDetails
		= renderPass.GetPipelineDetails();

	ID3D12CommandSignature* commandSignature = m_commandSignature.Get();

	for (const D3DExternalRenderPass::PipelineDetails& details : pipelineDetails)
	{
		const std::vector<std::uint32_t>& bundleIndices        = details.modelBundleIndices;
		const std::vector<std::uint32_t>& pipelineLocalIndices = details.pipelineLocalIndices;

		m_graphicsPipelineManager.BindPipeline(details.pipelineGlobalIndex, graphicsCmdList);

		const size_t bundleCount = std::size(bundleIndices);

		for (size_t index = 0u; index < bundleCount; ++index)
			m_modelManager->DrawPipeline(
				frameIndex, bundleIndices[index], pipelineLocalIndices[index],
				graphicsCmdList, commandSignature, m_meshManager
			);
	}
}

void RenderEngineVSIndirect::DrawingStage(
	size_t frameIndex, ID3D12Resource* swapchainBackBuffer, UINT64& counterValue, ID3D12Fence* waitFence
) {
	// Graphics Phase
	const D3DCommandList& graphicsCmdList = m_graphicsQueue.GetCommandList(frameIndex);

	{
		const CommandListScope graphicsCmdListScope{ graphicsCmdList };

		m_textureStorage.TransitionQueuedTextures(graphicsCmdListScope);

		m_viewportAndScissors.Bind(graphicsCmdListScope);

		m_graphicsDescriptorManagers[frameIndex].BindDescriptorHeap(graphicsCmdListScope);

		m_graphicsRootSignature.BindToGraphics(graphicsCmdListScope);

		m_graphicsDescriptorManagers[frameIndex].BindDescriptors(graphicsCmdListScope);

		// Normal passes
		const size_t renderPassCount = std::size(m_renderPasses);

		for (size_t index = 0u; index < renderPassCount; ++index)
		{
			if (!m_renderPasses.IsInUse(index))
				continue;

			const ExternalRenderPass_t& renderPass = *m_renderPasses[index];

			renderPass.StartPass(graphicsCmdListScope);

			DrawRenderPassPipelines(frameIndex, graphicsCmdListScope, renderPass);

			// No need to end passes for non swapchain passes.
		}

		// The one for the swapchain
		if (m_swapchainRenderPass)
		{
			const ExternalRenderPass_t& renderPass = *m_swapchainRenderPass;

			renderPass.StartPass(graphicsCmdListScope);

			DrawRenderPassPipelines(frameIndex, graphicsCmdListScope, renderPass);

			renderPass.EndPassForSwapchain(graphicsCmdListScope, swapchainBackBuffer);
		}
	}

	{
		const D3DFence& graphicsWaitFence = m_graphicsWait[frameIndex];

		const UINT64 oldCounterValue = counterValue;
		++counterValue;

		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalFence(graphicsWaitFence, counterValue)
			.WaitFence(waitFence, oldCounterValue)
			.CommandList(graphicsCmdList);

		m_graphicsQueue.SubmitCommandLists(graphicsSubmitBuilder);
	}
}
