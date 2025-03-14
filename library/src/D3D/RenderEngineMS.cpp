#include <RenderEngineMS.hpp>

RenderEngineMS::RenderEngineMS(
	const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount, ModelManagerMS{} }
{
	SetGraphicsDescriptorBufferLayout();

	m_cameraManager.CreateBuffer(static_cast<std::uint32_t>(frameCount));
}

void RenderEngineMS::FinaliseInitialisation(const DeviceManager& deviceManager)
{
	m_externalResourceManager.SetGraphicsDescriptorLayout(m_graphicsDescriptorManagers);

	for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
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
			RSCompileFlagBuilder{}.MeshShader(), BindlessLevel::UnboundArray
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

void RenderEngineMS::SetGraphicsDescriptorBufferLayout()
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorLayout(m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace);
	m_meshManager.SetDescriptorLayout(m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace);

	SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY_ALL); // AS, MS and PS all of them will use it.

	for (D3DDescriptorManager& descriptorManager : m_graphicsDescriptorManagers)
	{
		descriptorManager.AddRootSRV(
			s_modelBuffersGraphicsSRVRegisterSlot, s_vertexShaderRegisterSpace,
			D3D12_SHADER_VISIBILITY_ALL
		); // Both the AS and MS will use it.
		descriptorManager.AddRootSRV(
			s_modelBuffersPixelSRVRegisterSlot, s_pixelShaderRegisterSpace, D3D12_SHADER_VISIBILITY_PIXEL
		);
	}
}

void RenderEngineMS::ExecutePipelineStages(
	size_t frameIndex, ID3D12Resource* swapchainBackBuffer, UINT64& counterValue,
	ID3D12Fence* waitFence
) {
	waitFence = GenericCopyStage(frameIndex, counterValue, waitFence);

	DrawingStage(frameIndex, swapchainBackBuffer, counterValue, waitFence);
}

void RenderEngineMS::SetGraphicsDescriptors()
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
			descriptorManager, frameIndex, s_modelBuffersPixelSRVRegisterSlot, s_pixelShaderRegisterSpace
		);
	}
}

std::uint32_t RenderEngineMS::AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle)
{
	WaitForGPUToFinish();

	std::vector<std::uint32_t> modelBufferIndices = AddModelsToBuffer(*modelBundle, m_modelBuffers);

	const std::uint32_t index = m_modelManager.AddModelBundle(
		std::move(modelBundle), std::move(modelBufferIndices)
	);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	SetGraphicsDescriptors();

	m_copyNecessary = true;

	return index;
}

void RenderEngineMS::RemoveModelBundle(std::uint32_t bundleIndex) noexcept
{
	std::vector<std::uint32_t> modelBufferIndices = m_modelManager.RemoveModelBundle(bundleIndex);

	m_modelBuffers.Remove(modelBufferIndices);
}

std::uint32_t RenderEngineMS::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
{
	WaitForGPUToFinish();

	const std::uint32_t index = m_meshManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);

	m_meshManager.SetDescriptors(m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace);

	m_copyNecessary = true;

	return index;
}

ID3D12Fence* RenderEngineMS::GenericCopyStage(
	size_t frameIndex, UINT64& counterValue, ID3D12Fence* waitFence
) {
	// Copy Phase
	// If the Copy stage isn't executed, pass the waitFence on.
	ID3D12Fence* signalledFence = waitFence;

	// Only execute this stage if copying is necessary.
	if (m_copyNecessary)
	{
		const D3DCommandList& copyCmdList = m_copyQueue.GetCommandList(frameIndex);

		{
			const CommandListScope copyCmdListScope{ copyCmdList };

			// Need to copy the old buffers first to avoid empty data being copied over
			// the queued data.
			m_externalResourceManager.CopyQueuedBuffers(copyCmdListScope);
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

void RenderEngineMS::DrawRenderPassPipelines(
	const D3DCommandList& graphicsCmdList, const ExternalRenderPass_t& renderPass
) noexcept {
	const std::vector<D3DExternalRenderPass::PipelineDetails>& pipelineDetails
		= renderPass.GetPipelineDetails();

	for (const D3DExternalRenderPass::PipelineDetails& details : pipelineDetails)
	{
		const std::vector<std::uint32_t>& bundleIndices        = details.modelBundleIndices;
		const std::vector<std::uint32_t>& pipelineLocalIndices = details.pipelineLocalIndices;

		m_graphicsPipelineManager.BindPipeline(details.pipelineGlobalIndex, graphicsCmdList);

		const size_t bundleCount = std::size(bundleIndices);

		if (details.renderSorted)
			for (size_t index = 0u; index < bundleCount; ++index)
				m_modelManager.DrawPipelineSorted(
					bundleIndices[index], pipelineLocalIndices[index],
					graphicsCmdList, m_meshManager
				);
		else
			for (size_t index = 0u; index < bundleCount; ++index)
				m_modelManager.DrawPipeline(
					bundleIndices[index], pipelineLocalIndices[index],
					graphicsCmdList, m_meshManager
				);
	}
}

void RenderEngineMS::DrawingStage(
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
