#include <RenderEngineMeshShader.hpp>

RenderEngineMS::RenderEngineMS(
	const DeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorLayout(
		m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace, s_pixelShaderRegisterSpace
	);
	SetCommonGraphicsDescriptorLayout(D3D12_SHADER_VISIBILITY_ALL); // Both the AS and MS will use it.

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
			RSCompileFlagBuilder{}.MeshShader(), BindlessLevel::UnboundArray
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

void RenderEngineMS::SetupPipelineStages()
{
	constexpr size_t stageCount = 2u;

	m_pipelineStages.reserve(stageCount);

	m_pipelineStages.emplace_back(&RenderEngineMS::GenericCopyStage);
	m_pipelineStages.emplace_back(&RenderEngineMS::DrawingStage);
}

std::uint32_t RenderEngineMS::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& pixelShader
) {
	WaitForGPUToFinish();

	const std::uint32_t index = m_modelManager.AddModelBundle(
		std::move(modelBundle), pixelShader, m_temporaryDataBuffer
	);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorsOfModels(
		m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace, s_pixelShaderRegisterSpace
	);

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineMS::AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle)
{
	WaitForGPUToFinish();

	const std::uint32_t index = m_modelManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);

	m_modelManager.SetDescriptorsOfMeshes(m_graphicsDescriptorManagers, s_vertexShaderRegisterSpace);

	m_copyNecessary = true;

	return index;
}

ID3D12Fence* RenderEngineMS::GenericCopyStage(
	size_t frameIndex,
	[[maybe_unused]] const RenderTarget& renderTarget, UINT64& counterValue, ID3D12Fence* waitFence
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

ID3D12Fence* RenderEngineMS::DrawingStage(
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

ModelManagerMS RenderEngineMS::GetModelManager(
	const DeviceManager& deviceManager, MemoryManager* memoryManager,
	StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
) {
	return ModelManagerMS{
		deviceManager.GetDevice(), memoryManager, stagingBufferMan, frameCount
	};
}
