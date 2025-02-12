#include <ModelManager.hpp>
#include <D3DRootSignatureDynamic.hpp>

// Model Manager VS Individual
ModelManagerVSIndividual::ModelManagerVSIndividual(MemoryManager* memoryManager)
	: ModelManager{ memoryManager }, m_constantsRootIndex{ 0u }
{}

void ModelManagerVSIndividual::_setGraphicsConstantRootIndex(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) noexcept {
	m_constantsRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataCBVRegisterSlot, constantsRegisterSpace
	);
}

void ModelManagerVSIndividual::ConfigureModelBundle(
	ModelBundleVSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundle>&& modelBundle
) const noexcept {
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));
}

void ModelManagerVSIndividual::ConfigureModelBundleRemove(
	size_t bundleIndex, ModelBuffers& modelBuffers
) noexcept {
	const ModelBundleVSIndividual& modelBundle     = m_modelBundles.at(bundleIndex);
	const std::vector<std::uint32_t>& modelIndices = modelBundle.GetModelIndices();

	for (std::uint32_t modelIndex : modelIndices)
		modelBuffers.Remove(modelIndex);
}

void ModelManagerVSIndividual::SetDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
) {
	constexpr UINT pushConstantCount = ModelBundleVSIndividual::GetConstantCount();

	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
		descriptorManager.AddConstants(
			s_constantDataCBVRegisterSlot, vsRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
}

void ModelManagerVSIndividual::Draw(
	const D3DCommandList& graphicsList, const MeshManagerVSIndividual& meshManager,
	const PipelineManager<Pipeline_t>& pipelineManager
) const noexcept {
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	GraphicsPipelineVS::SetIATopology(graphicsList);

	for (const ModelBundleVSIndividual& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsList, pipelineManager, previousPSOIndex);

		// Mesh
		const D3DMeshBundleVS& meshBundle = meshManager.GetBundle(
			static_cast<size_t>(modelBundle.GetMeshBundleIndex())
		);

		meshBundle.Bind(graphicsList);

		// Model
		modelBundle.Draw(graphicsList, m_constantsRootIndex, meshBundle);
	}
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	ID3D12Device5* device, MemoryManager* memoryManager, std::uint32_t frameCount
) : ModelManager{ memoryManager },
	m_argumentInputBuffers{}, m_argumentOutputBuffers{},
	m_cullingDataBuffer{ device, memoryManager, D3D12_RESOURCE_STATE_GENERIC_READ },
	m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
	m_meshBundleIndexBuffer{ device, memoryManager, frameCount },
	m_perModelDataBuffer{ device, memoryManager, D3D12_RESOURCE_STATE_GENERIC_READ },
	m_dispatchXCount{ 0u }, m_argumentCount{ 0u }, m_constantsVSRootIndex{ 0u },
	m_constantsCSRootIndex{ 0u }, m_modelBundlesCS{}
{
	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_argumentInputBuffers.emplace_back(
			SharedBufferCPU{ device, m_memoryManager, D3D12_RESOURCE_STATE_GENERIC_READ }
		);
		m_argumentOutputBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, m_memoryManager, D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			}
		);
		// Doing the resetting on the Compute queue, so CG should be fine.
		m_counterBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, m_memoryManager, D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			}
		);
	}
}

void ModelManagerVSIndirect::_setGraphicsConstantRootIndex(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) noexcept {
	m_constantsVSRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataVSCBVRegisterSlot, constantsRegisterSpace
	);
}

void ModelManagerVSIndirect::SetComputeConstantRootIndex(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) noexcept {
	m_constantsVSRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataCSCBVRegisterSlot, constantsRegisterSpace
	);
}

void ModelManagerVSIndirect::UpdateDispatchX() noexcept
{
	// ThreadBlockSize is the number of threads in a thread group. If the argumentCount/ModelCount
	// is more than the BlockSize then dispatch more groups. Ex: Threads 64, Model 60 = Group 1
	// Threads 64, Model 65 = Group 2.
	m_dispatchXCount = static_cast<UINT>(std::ceil(m_argumentCount / THREADBLOCKSIZE));
}

void ModelManagerVSIndirect::ConfigureModelBundle(
	ModelBundleVSIndirect& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundle>&& modelBundle
) {
	ModelBundleCSIndirect modelBundleCS{};

	modelBundleCS.SetModelBundle(modelBundle, std::move(modelIndices));
	modelBundleObj.SetModelBundle(std::move(modelBundle));

	modelBundleCS.CreateBuffers(m_argumentInputBuffers, m_cullingDataBuffer, m_perModelDataBuffer);

	modelBundleObj.CreateBuffers(
		m_argumentOutputBuffers, m_counterBuffers, modelBundleCS.GetModelCount()
	);

	UpdateCounterResetValues();

	modelBundleObj.SetID(static_cast<std::uint32_t>(modelBundleCS.GetID()));

	const std::uint32_t modelBundleIndexInBuffer = modelBundleCS.GetModelBundleIndex();

	m_meshBundleIndexBuffer.Add(modelBundleIndexInBuffer);

	m_argumentCount += modelBundleCS.GetModelCount();

	m_modelBundlesCS.emplace_back(std::move(modelBundleCS));

	UpdateDispatchX();
}

void ModelManagerVSIndirect::ConfigureModelBundleRemove(
	size_t bundleIndex, ModelBuffers& modelBuffers
) noexcept {
	const ModelBundleVSIndirect& modelBundleVS = m_modelBundles[bundleIndex];

	{
		const std::vector<SharedBufferData>& argumentOutputSharedData
			= modelBundleVS.GetArgumentOutputSharedData();

		for (size_t index = 0u; index < std::size(m_argumentOutputBuffers); ++index)
			m_argumentOutputBuffers[index].RelinquishMemory(argumentOutputSharedData[index]);

		const std::vector<SharedBufferData>& counterSharedData = modelBundleVS.GetCounterSharedData();

		for (size_t index = 0u; index < std::size(m_counterBuffers); ++index)
			m_counterBuffers[index].RelinquishMemory(counterSharedData[index]);
	}

	// The index should be the same as the VS one.
	const ModelBundleCSIndirect& modelBundleCS = m_modelBundlesCS[bundleIndex];

	{
		const std::vector<SharedBufferData>& argumentInputSharedData
			= modelBundleCS.GetArgumentInputSharedData();

		for (size_t index = 0u; index < std::size(m_argumentInputBuffers); ++index)
			m_argumentInputBuffers[index].RelinquishMemory(argumentInputSharedData[index]);

		modelBundleCS.ResetCullingData();

		m_cullingDataBuffer.RelinquishMemory(modelBundleCS.GetCullingSharedData());
		m_perModelDataBuffer.RelinquishMemory(modelBundleCS.GetPerModelDataSharedData());

		// Remove the model indices
		const std::vector<std::uint32_t>& modelIndices = modelBundleCS.GetModelIndices();

		for (std::uint32_t modelIndex : modelIndices)
			modelBuffers.Remove(modelIndex);
	}

	m_modelBundlesCS.erase(std::next(std::begin(m_modelBundlesCS), bundleIndex));
}

void ModelManagerVSIndirect::UpdatePerFrame(
	UINT64 frameIndex, const MeshManagerVSIndirect& meshManager
) const noexcept {
	std::uint8_t* bufferOffsetPtr = m_meshBundleIndexBuffer.GetInstancePtr(frameIndex);
	constexpr size_t strideSize   = sizeof(std::uint32_t);
	size_t bufferOffset           = 0u;

	// This is necessary because, while the indices should be the same as the CS bundles
	// if we decide to remove a bundle, the bundles afterwards will be shifted. In that
	// case either we can shift the modelBundleIndices to accommodate the bundle changes or
	// write data the modelBundles' old position. I think the second approach is better
	// as the model bundle indices are currently set as GPU only data. While the mesh indices
	// are cpu data which is reset every frame.
	for (const ModelBundleCSIndirect& bundle : m_modelBundlesCS)
	{
		const std::uint32_t meshBundleIndex = bundle.GetMeshBundleIndex();

		const D3DMeshBundleVS& meshBundle   = meshManager.GetBundle(static_cast<size_t>(meshBundleIndex));

		bundle.Update(static_cast<size_t>(frameIndex), meshBundle);

		const std::uint32_t modelBundleIndex = bundle.GetModelBundleIndex();

		bufferOffset = strideSize * modelBundleIndex;

		memcpy(bufferOffsetPtr + bufferOffset, &meshBundleIndex, strideSize);
	}
}

void ModelManagerVSIndirect::SetDescriptorLayoutVS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
) const noexcept {
	constexpr auto pushConstantCount = ModelBundleVSIndirect::GetConstantCount();

	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
		descriptorManager.AddConstants(
			s_constantDataVSCBVRegisterSlot, vsRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
}

void ModelManagerVSIndirect::SetDescriptorLayoutCS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const noexcept {
	constexpr auto pushConstantCount = GetConstantCount();

	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
	{
		descriptorManager.AddConstants(
			s_constantDataCSCBVRegisterSlot, csRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_argumentInputBufferSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_cullingDataBufferSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootUAV(
			s_argumenOutputUAVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootUAV(
			s_counterUAVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_perModelDataSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_meshBundleIndexSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
	}
}

void ModelManagerVSIndirect::SetDescriptors(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const {
	const size_t frameCount = std::size(descriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];

		descriptorManager.SetRootSRV(
			s_argumentInputBufferSRVRegisterSlot, csRegisterSpace,
			m_argumentInputBuffers[index].GetGPUAddress(), false
		);
		descriptorManager.SetRootSRV(
			s_cullingDataBufferSRVRegisterSlot, csRegisterSpace,
			m_cullingDataBuffer.GetGPUAddress(), false
		);
		descriptorManager.SetRootUAV(
			s_argumenOutputUAVRegisterSlot, csRegisterSpace,
			m_argumentOutputBuffers[index].GetGPUAddress(), false
		);
		descriptorManager.SetRootUAV(
			s_counterUAVRegisterSlot, csRegisterSpace, m_counterBuffers[index].GetGPUAddress(), false
		);
		descriptorManager.SetRootSRV(
			s_perModelDataSRVRegisterSlot, csRegisterSpace,
			m_perModelDataBuffer.GetGPUAddress(), false
		);

		m_meshBundleIndexBuffer.SetRootSRVCom(
			descriptorManager, s_meshBundleIndexSRVRegisterSlot, csRegisterSpace
		);
	}
}

void ModelManagerVSIndirect::Dispatch(
	const D3DCommandList& computeList, const PipelineManager<ComputePipeline_t>& pipelineManager
) const noexcept {
	ID3D12GraphicsCommandList* cmdList      = computeList.Get();

	// There should be a single one for now.
	static constexpr size_t computePSOIndex = 0u;

	pipelineManager.BindPipeline(computePSOIndex, computeList);

	{
		constexpr UINT pushConstantCount = GetConstantCount();

		const ConstantData constantData
		{
			.modelCount = m_argumentCount
		};

		cmdList->SetComputeRoot32BitConstants(
			m_constantsCSRootIndex, pushConstantCount, &constantData, 0u
		);
	}

	cmdList->Dispatch(m_dispatchXCount, 1u, 1u);
}

void ModelManagerVSIndirect::Draw(
	size_t frameIndex, const D3DCommandList& graphicsList, ID3D12CommandSignature* commandSignature,
	const MeshManagerVSIndirect& meshManager, const PipelineManager<GraphicsPipeline_t>& pipelineManager
) const noexcept {
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	GraphicsPipelineVS::SetIATopology(graphicsList);

	for (const ModelBundleVSIndirect& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsList, pipelineManager, previousPSOIndex);

		// Mesh
		const D3DMeshBundleVS& meshBundle = meshManager.GetBundle(
			static_cast<size_t>(modelBundle.GetMeshBundleIndex())
		);

		meshBundle.Bind(graphicsList);

		// Model
		modelBundle.Draw(frameIndex, commandSignature, graphicsList);
	}
}

void ModelManagerVSIndirect::ResetCounterBuffer(
	const D3DCommandList& computeList, size_t frameIndex
) const noexcept {
	const SharedBufferGPUWriteOnly& counterBuffer = m_counterBuffers[frameIndex];

	computeList.CopyWhole(m_counterResetBuffer, counterBuffer.GetBuffer());

	D3DResourceBarrier{}.AddBarrier(
		ResourceBarrierBuilder{}
		.Transition(
			counterBuffer.GetBuffer().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		)
	).RecordBarriers(computeList.Get());
}

void ModelManagerVSIndirect::UpdateCounterResetValues()
{
	if (!std::empty(m_counterBuffers))
	{
		const SharedBufferGPUWriteOnly& counterBuffer = m_counterBuffers.front();

		const UINT64 counterBufferSize = counterBuffer.Size();
		const UINT64 oldCounterSize    = m_counterResetBuffer.BufferSize();

		if (counterBufferSize > oldCounterSize)
		{
			const size_t counterSize = sizeof(std::uint32_t);

			// This should be the source buffer. And should only be accessed from a single type of
			// queue.
			m_counterResetBuffer.Create(counterBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);

			constexpr std::uint32_t value = 0u;

			std::uint8_t* bufferStart = m_counterResetBuffer.CPUHandle();

			for (size_t offset = 0u; offset < counterBufferSize; offset += counterSize)
				memcpy(bufferStart + offset, &value, counterSize);
		}
	}
}

// Model Manager MS.
ModelManagerMS::ModelManagerMS(MemoryManager* memoryManager)
	: ModelManager{ memoryManager }, m_constantsRootIndex{ 0u }
{}

void ModelManagerMS::ConfigureModelBundle(
	ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundle>&& modelBundle
) {
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));
}

void ModelManagerMS::ConfigureModelBundleRemove(size_t bundleIndex, ModelBuffers& modelBuffers) noexcept
{
	const ModelBundleMSIndividual& modelBundle     = m_modelBundles.at(bundleIndex);
	const std::vector<std::uint32_t>& modelIndices = modelBundle.GetModelIndices();

	for (std::uint32_t modelIndex : modelIndices)
		modelBuffers.Remove(modelIndex);
}

void ModelManagerMS::_setGraphicsConstantRootIndex(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) noexcept {
	m_constantsRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataCBVRegisterSlot, constantsRegisterSpace
	);
}

void ModelManagerMS::SetDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
) const noexcept {
	constexpr UINT meshConstantCount  = D3DMeshBundleMS::GetConstantCount();
	constexpr UINT modelConstantCount = ModelBundleMSIndividual::GetConstantCount();

	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
		descriptorManager.AddConstants(
			s_constantDataCBVRegisterSlot, msRegisterSpace, meshConstantCount + modelConstantCount,
			D3D12_SHADER_VISIBILITY_ALL
		); // Both the AS and MS will use it.
}

void ModelManagerMS::Draw(
	const D3DCommandList& graphicsList, const MeshManagerMS& meshManager,
	const PipelineManager<Pipeline_t>& pipelineManager
) const noexcept {
	auto previousPSOIndex              = std::numeric_limits<size_t>::max();
	ID3D12GraphicsCommandList* cmdList = graphicsList.Get();

	for (const ModelBundleMSIndividual& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsList, pipelineManager, previousPSOIndex);

		const auto meshBundleIndex        = static_cast<size_t>(
			modelBundle.GetMeshBundleIndex()
		);
		const D3DMeshBundleMS& meshBundle = meshManager.GetBundle(meshBundleIndex);

		constexpr UINT constBufferCount   = D3DMeshBundleMS::GetConstantCount();

		const D3DMeshBundleMS::MeshBundleDetailsMS meshBundleDetailsMS
			= meshBundle.GetMeshBundleDetailsMS();

		cmdList->SetGraphicsRoot32BitConstants(
			m_constantsRootIndex, constBufferCount, &meshBundleDetailsMS, 0u
		);

		// Model
		modelBundle.Draw(graphicsList, m_constantsRootIndex, meshBundle);
	}
}
