#include <RenderEngineMS.hpp>

RenderEngineMS::RenderEngineMS(
	const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	SetGraphicsDescriptorBufferLayout();

	m_cameraManager.CreateBuffer(static_cast<std::uint32_t>(frameCount));
}

void RenderEngineMS::FinaliseInitialisation(const DeviceManager& deviceManager)
{
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

		m_renderPassManager.SetRootSignature(m_graphicsRootSignature.Get());
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
	SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY_ALL); // Both the AS and MS will use it.

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
	size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue,
	ID3D12Fence* waitFence
) {
	waitFence = GenericCopyStage(frameIndex, counterValue, waitFence);

	DrawingStage(frameIndex, renderTarget, counterValue, waitFence);
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

std::uint32_t RenderEngineMS::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
) {
	WaitForGPUToFinish();

	const std::uint32_t psoIndex = m_renderPassManager.AddOrGetGraphicsPipeline(pixelShader);

	const std::uint32_t index    = m_modelManager.AddModelBundle(
		std::move(modelBundle), psoIndex, m_modelBuffers, m_stagingManager, m_temporaryDataBuffer
	);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	SetGraphicsDescriptors();

	m_copyNecessary = true;

	return index;
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

void RenderEngineMS::DrawingStage(
	size_t frameIndex, const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
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

		m_renderPassManager.BeginRenderingWithDepth(
			graphicsCmdListScope, renderTarget, m_backgroundColour
		);

		m_modelManager.Draw(
			graphicsCmdListScope, m_meshManager, m_renderPassManager.GetGraphicsPipelineManager()
		);

		m_renderPassManager.EndRendering(graphicsCmdListScope, renderTarget);
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

ModelManagerMS RenderEngineMS::GetModelManager(
	[[maybe_unused]] const DeviceManager& deviceManager,
	MemoryManager* memoryManager,
	[[maybe_unused]] std::uint32_t frameCount
) {
	return ModelManagerMS{ memoryManager };
}
