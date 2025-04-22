#include <unordered_map>
#include <D3DModelManager.hpp>
#include <D3DRootSignatureDynamic.hpp>

namespace Gaia
{
// Model Manager VS Individual
void ModelManagerVSIndividual::SetGraphicsConstantsRootIndex(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) noexcept {
	m_constantsRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataCBVRegisterSlot, constantsRegisterSpace
	);
}

void ModelManagerVSIndividual::SetDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
) {
	constexpr UINT pushConstantCount = PipelineModelsVSIndividual::GetConstantCount();

	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
		descriptorManager.AddConstants(
			s_constantDataCBVRegisterSlot, vsRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
}

void ModelManagerVSIndividual::DrawPipeline(
	size_t modelBundleIndex, size_t pipelineLocalIndex, const D3DCommandList& graphicsList,
	const MeshManagerVSIndividual& meshManager
) const noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	GraphicsPipelineVS::SetIATopology(graphicsList);

	const ModelBundleVSIndividual& modelBundle = m_modelBundles[modelBundleIndex];

	// Mesh
	const D3DMeshBundleVS& meshBundle          = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	// Model
	modelBundle.DrawPipeline(pipelineLocalIndex, graphicsList, m_constantsRootIndex, meshBundle);
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	ID3D12Device5* device, MemoryManager* memoryManager, std::uint32_t frameCount
) : ModelManager{},
	m_argumentInputBuffers{}, m_argumentOutputBuffers{},
	m_perPipelineBuffer{ device, memoryManager, D3D12_RESOURCE_STATE_GENERIC_READ },
	m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
	m_perModelBundleBuffer{ device, memoryManager, frameCount },
	m_perModelBuffer{ device, memoryManager, D3D12_RESOURCE_STATE_GENERIC_READ },
	m_dispatchXCount{ 0u }, m_allocatedModelCount{ 0u }, m_csPSOIndex{ 0u }, m_constantsVSRootIndex{ 0u },
	m_constantsCSRootIndex{ 0u }
{
	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_argumentInputBuffers.emplace_back(
			SharedBufferCPU{ device, memoryManager, D3D12_RESOURCE_STATE_GENERIC_READ }
		);
		m_argumentOutputBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, memoryManager, D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			}
		);
		// Doing the resetting on the Compute queue, so CG should be fine.
		m_counterBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, memoryManager, D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			}
		);
	}
}

void ModelManagerVSIndirect::SetGraphicsConstantsRootIndex(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) noexcept {
	m_constantsVSRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataVSCBVRegisterSlot, constantsRegisterSpace
	);
}

void ModelManagerVSIndirect::SetComputeConstantsRootIndex(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) noexcept {
	m_constantsVSRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataCSCBVRegisterSlot, constantsRegisterSpace
	);
}

void ModelManagerVSIndirect::ReconfigureModels(
	std::uint32_t bundleIndex, std::uint32_t decreasedModelsPipelineIndex,
	std::uint32_t increasedModelsPipelineIndex
) {
	m_modelBundles[bundleIndex].ReconfigureModels(
		bundleIndex, decreasedModelsPipelineIndex, increasedModelsPipelineIndex,
		m_argumentInputBuffers, m_perPipelineBuffer, m_perModelBuffer,
		m_argumentOutputBuffers, m_counterBuffers
	);
}

void ModelManagerVSIndirect::UpdateAllocatedModelCount() noexcept
{
	// We can't reduce the amount of allocated model count, as we don't deallocate. We only
	// make the memory available for something else. So, if a bundle from the middle is freed
	// and we decrease the model count, then the last ones will be the ones which won't be rendered.
	// Then again we can't also keep adding the newly added model count, as they might be allocated
	// in some freed memory. We should set the model count to the total allocated model count, that
	// way it won't skip the last ones and also not unnecessarily add extra ones.
	m_allocatedModelCount = static_cast<UINT>(
		m_perModelBuffer.Size() / PipelineModelsCSIndirect::GetPerModelStride()
	);

	// ThreadBlockSize is the number of threads in a thread group. If the allocated model count
	// is more than the BlockSize then dispatch more groups. Ex: Threads 64, Model 60 = Group 1
	// Threads 64, Model 65 = Group 2.
	m_dispatchXCount = static_cast<UINT>(std::ceil(m_allocatedModelCount / THREADBLOCKSIZE));
}

std::uint32_t ModelManagerVSIndirect::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle,
	const std::vector<std::uint32_t>& modelIndicesInBuffer
) {
	const size_t bundleIndex  = m_modelBundles.Add(ModelBundleVSIndirect{});
	const auto bundleIndexU32 = static_cast<std::uint32_t>(bundleIndex);

	ModelBundleVSIndirect& localModelBundle = m_modelBundles[bundleIndex];

	SetModelIndicesInBuffer(*modelBundle, modelIndicesInBuffer);

	localModelBundle.SetModelBundle(std::move(modelBundle));

	localModelBundle.AddNewPipelinesFromBundle(
		bundleIndexU32, m_argumentInputBuffers, m_perPipelineBuffer, m_perModelBuffer,
		m_argumentOutputBuffers, m_counterBuffers
	);

	UpdateCounterResetValues();

	m_perModelBundleBuffer.ExtendBufferIfNecessaryFor(bundleIndex);

	UpdateAllocatedModelCount();

	return bundleIndexU32;
}

std::shared_ptr<ModelBundle> ModelManagerVSIndirect::RemoveModelBundle(
	std::uint32_t bundleIndex
) noexcept {
	const size_t bundleIndexST = bundleIndex;

	ModelBundleVSIndirect& localModelBundle = m_modelBundles[bundleIndexST];

	std::shared_ptr<ModelBundle> modelBundle = localModelBundle.GetModelBundle();

	localModelBundle.CleanupData(
		m_argumentInputBuffers, m_perPipelineBuffer, m_perModelBuffer,
		m_argumentOutputBuffers, m_counterBuffers
	);

	m_modelBundles.RemoveElement(bundleIndexST);

	return modelBundle;
}

void ModelManagerVSIndirect::UpdatePipelinePerFrame(
	UINT64 frameIndex, size_t modelBundleIndex, size_t pipelineLocalIndex,
	const MeshManagerVSIndirect& meshManager, bool skipCulling
) const noexcept {
	std::uint8_t* bufferOffsetPtr = m_perModelBundleBuffer.GetInstancePtr(frameIndex);
	constexpr size_t strideSize   = sizeof(PerModelBundleData);

	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	const size_t bufferOffset             = strideSize * modelBundleIndex;

	const ModelBundleVSIndirect& vsBundle = m_modelBundles[modelBundleIndex];

	const std::uint32_t meshBundleIndex   = vsBundle.GetMeshBundleIndex();

	const D3DMeshBundleVS& meshBundle     = meshManager.GetBundle(meshBundleIndex);

	vsBundle.UpdatePipeline(
		pipelineLocalIndex, static_cast<size_t>(frameIndex), meshBundle, skipCulling
	);

	memcpy(bufferOffsetPtr + bufferOffset, &meshBundleIndex, strideSize);
}

void ModelManagerVSIndirect::SetDescriptorLayoutVS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace
) const noexcept {
	constexpr UINT pushConstantCount = PipelineModelsVSIndirect::GetConstantCount();

	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
		descriptorManager.AddConstants(
			s_constantDataVSCBVRegisterSlot, vsRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
}

void ModelManagerVSIndirect::SetDescriptorLayoutCS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const noexcept {
	constexpr UINT pushConstantCount = GetConstantCount();

	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
	{
		descriptorManager.AddConstants(
			s_constantDataCSCBVRegisterSlot, csRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_argumentInputSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_perPipelineSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootUAV(
			s_argumenOutputUAVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootUAV(
			s_counterUAVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_perModelSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_perModelBundleSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
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
			s_argumentInputSRVRegisterSlot, csRegisterSpace,
			m_argumentInputBuffers[index].GetGPUAddress(), false
		);
		descriptorManager.SetRootSRV(
			s_perPipelineSRVRegisterSlot, csRegisterSpace, m_perPipelineBuffer.GetGPUAddress(),
			false
		);
		descriptorManager.SetRootUAV(
			s_argumenOutputUAVRegisterSlot, csRegisterSpace,
			m_argumentOutputBuffers[index].GetGPUAddress(), false
		);
		descriptorManager.SetRootUAV(
			s_counterUAVRegisterSlot, csRegisterSpace, m_counterBuffers[index].GetGPUAddress(),
			false
		);
		descriptorManager.SetRootSRV(
			s_perModelSRVRegisterSlot, csRegisterSpace, m_perModelBuffer.GetGPUAddress(), false
		);

		m_perModelBundleBuffer.SetRootSRVCom(
			descriptorManager, s_perModelBundleSRVRegisterSlot, csRegisterSpace
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
			.allocatedModelCount = m_allocatedModelCount
		};

		cmdList->SetComputeRoot32BitConstants(
			m_constantsCSRootIndex, pushConstantCount, &constantData, 0u
		);
	}

	cmdList->Dispatch(m_dispatchXCount, 1u, 1u);
}

void ModelManagerVSIndirect::DrawPipeline(
	size_t frameIndex, size_t modelBundleIndex, size_t pipelineLocalIndex,
	const D3DCommandList& graphicsList, ID3D12CommandSignature* commandSignature,
	const MeshManagerVSIndirect& meshManager
) const noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	GraphicsPipelineVS::SetIATopology(graphicsList);

	const ModelBundleVSIndirect& modelBundle = m_modelBundles[modelBundleIndex];

	// Mesh
	const D3DMeshBundleVS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	// Model
	modelBundle.DrawPipeline(
		pipelineLocalIndex, frameIndex, graphicsList, commandSignature, meshBundle
	);
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
void ModelManagerMS::SetGraphicsConstantsRootIndex(
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
	constexpr UINT modelConstantCount = PipelineModelsMSIndividual::GetConstantCount();

	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
		descriptorManager.AddConstants(
			s_constantDataCBVRegisterSlot, msRegisterSpace, meshConstantCount + modelConstantCount,
			D3D12_SHADER_VISIBILITY_ALL
		); // Both the AS and MS will use it.
}

void ModelManagerMS::DrawPipeline(
	size_t modelBundleIndex, size_t pipelineLocalIndex, const D3DCommandList& graphicsList,
	const MeshManagerMS& meshManager
) const noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	const ModelBundleMSIndividual& modelBundle = m_modelBundles[modelBundleIndex];

	// Mesh
	const D3DMeshBundleMS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	// Model
	modelBundle.DrawPipeline(pipelineLocalIndex, graphicsList, m_constantsRootIndex, meshBundle);
}
}
