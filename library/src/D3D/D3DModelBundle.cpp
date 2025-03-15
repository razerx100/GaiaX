#include <D3DModelBundle.hpp>
#include <VectorToSharedPtr.hpp>

// Pipeline Models Base
D3D12_DRAW_INDEXED_ARGUMENTS PipelineModelsBase::GetDrawIndexedIndirectCommand(
	const MeshTemporaryDetailsVS& meshDetailsVS
) noexcept {
	const D3D12_DRAW_INDEXED_ARGUMENTS indirectCommand
	{
		.IndexCountPerInstance = meshDetailsVS.indexCount,
		.InstanceCount         = 1u,
		.StartIndexLocation    = meshDetailsVS.indexOffset,
		.BaseVertexLocation    = 0,
		.StartInstanceLocation = 0u
	};

	return indirectCommand;
}

uint32_t PipelineModelsBase::AddModel(std::uint32_t bundleIndex, std::uint32_t bufferIndex) noexcept
{
	return static_cast<std::uint32_t>(
		m_modelData.Add(ModelData{ .bundleIndex = bundleIndex, .bufferIndex = bufferIndex })
	);
}

// Pipeline Models VS Individual
void PipelineModelsVSIndividual::DrawModel(
	bool isInUse, const ModelData& modelData,
	ID3D12GraphicsCommandList* graphicsList, UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle,
	const std::vector<std::shared_ptr<Model>>& models
) const noexcept {
	const std::shared_ptr<Model>& model = models[modelData.bundleIndex];

	if (!isInUse || !model->IsVisible())
		return;

	constexpr UINT pushConstantCount = GetConstantCount();

	graphicsList->SetGraphicsRoot32BitConstants(
		constantsRootIndex, pushConstantCount, &modelData.bufferIndex, 0u
	);

	const MeshTemporaryDetailsVS& meshDetailsVS = meshBundle.GetMeshDetails(model->GetMeshIndex());
	const D3D12_DRAW_INDEXED_ARGUMENTS meshArgs = GetDrawIndexedIndirectCommand(meshDetailsVS);

	graphicsList->DrawIndexedInstanced(
		meshArgs.IndexCountPerInstance, meshArgs.InstanceCount, meshArgs.StartIndexLocation,
		meshArgs.BaseVertexLocation, meshArgs.StartInstanceLocation
	);
}

void PipelineModelsVSIndividual::Draw(
	const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle,
	const std::vector<std::shared_ptr<Model>>& models
) const noexcept {
	const size_t modelCount = std::size(m_modelData);

	ID3D12GraphicsCommandList* cmdList = graphicsList.Get();

	for (size_t index = 0; index < modelCount; ++index)
		DrawModel(
			m_modelData.IsInUse(index), m_modelData[index], cmdList, constantsRootIndex,
			meshBundle, models
		);
}

// Pipeline Models MS Individual
void PipelineModelsMSIndividual::DrawModel(
	bool isInUse, const ModelData& modelData, ID3D12GraphicsCommandList6* graphicsList,
	UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle,
	const std::vector<std::shared_ptr<Model>>& models
) const noexcept {
	const std::shared_ptr<Model>& model = models[modelData.bundleIndex];

	if (!isInUse || !model->IsVisible())
		return;

	constexpr UINT pushConstantCount = GetConstantCount();
	constexpr UINT constBufferOffset = D3DMeshBundleMS::GetConstantCount();

	const MeshTemporaryDetailsMS& meshDetailsMS = meshBundle.GetMeshDetails(model->GetMeshIndex());

	const ModelDetailsMS constants
	{
		.meshDetails = MeshDetails
			{
				.meshletCount  = meshDetailsMS.meshletCount,
				.meshletOffset = meshDetailsMS.meshletOffset,
				.indexOffset   = meshDetailsMS.indexOffset,
				.primOffset    = meshDetailsMS.primitiveOffset,
				.vertexOffset  = meshDetailsMS.vertexOffset,
			},
		.modelBufferIndex = modelData.bufferIndex
	};

	graphicsList->SetGraphicsRoot32BitConstants(
		constantsRootIndex, pushConstantCount, &constants, constBufferOffset
	);

	// If we have an Amplification shader, this will launch an amplification global workGroup.
	// We would want each Amplification shader lane to process a meshlet and launch the necessary
	// Mesh Shader workGroups. On Nvdia we can have a maximum of 32 lanes active
	// in a wave and 64 on AMD. So, a workGroup will be able to work on 32/64
	// meshlets concurrently.
	const UINT amplficationGroupCount = DivRoundUp(
		meshDetailsMS.meshletCount, s_amplificationLaneCount
	);

	graphicsList->DispatchMesh(amplficationGroupCount, 1u, 1u);
	// It might be worth checking if we are reaching the Group Count Limit and if needed
	// launch more Groups. Could achieve that by passing a GroupLaunch index.
}

void PipelineModelsMSIndividual::Draw(
	const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle,
	const std::vector<std::shared_ptr<Model>>& models
) const noexcept {
	const size_t modelCount = std::size(m_modelData);

	ID3D12GraphicsCommandList6* cmdList = graphicsList.Get();

	for (size_t index = 0u; index < modelCount; ++index)
		DrawModel(
			m_modelData.IsInUse(index), m_modelData[index], cmdList, constantsRootIndex,
			meshBundle, models
		);
}

// Pipeline Models CS Indirect
PipelineModelsCSIndirect::PipelineModelsCSIndirect()
	: m_perPipelineSharedData{ nullptr, 0u, 0u }, m_perModelSharedData{ nullptr, 0u, 0u },
	m_argumentInputSharedData{}
{}

void PipelineModelsCSIndirect::ResetCullingData() const noexcept
{
	// Before destroying an object the model count needs to be set to 0,
	// so the models aren't processed in the compute shader anymore.
	if (m_perPipelineSharedData.bufferData)
	{
		std::uint32_t modelCount        = 0u;
		Buffer const* perPipelineBuffer = m_perPipelineSharedData.bufferData;

		memcpy(
			perPipelineBuffer->CPUHandle() + m_perPipelineSharedData.offset,
			&modelCount, sizeof(std::uint32_t)
		);
	}
}

std::vector<PipelineModelsCSIndirect::IndexLink> PipelineModelsCSIndirect::RemoveInactiveModels() noexcept
{
	std::vector<IndexLink> activeIndexLink
	{
		m_modelData.GetIndicesManager().GetActiveIndexCount(),
		IndexLink
		{
			.bundleIndex = std::numeric_limits<std::uint32_t>::max(),
			.localIndex  = std::numeric_limits<std::uint32_t>::max()
		}
	};

	m_modelData.EraseInactiveElements();

	for (size_t index = 0u; index < std::size(m_modelData); ++index)
	{
		activeIndexLink[index].bundleIndex = m_modelData[index].bundleIndex;
		activeIndexLink[index].localIndex  = static_cast<std::uint32_t>(index);
	}

	return activeIndexLink;
}

void PipelineModelsCSIndirect::UpdateNonPerFrameData(std::uint32_t modelBundleIndex) noexcept
{
	constexpr size_t argumentStrideSize = sizeof(IndirectArgument);
	constexpr size_t perModelStride     = sizeof(PerModelData);
	constexpr size_t perPipelineStride  = sizeof(PerPipelineData);

	const size_t modelCount = GetModelCount();

	if (!modelCount)
		return;

	// Per Pipeline Data.
	{
		PerPipelineData perPipelineData
		{
			.modelCount       = static_cast<std::uint32_t>(modelCount),
			.modelOffset      = 0u,
			.modelBundleIndex = modelBundleIndex
		};

		if (!std::empty(m_argumentInputSharedData))
		{
			const SharedBufferData& sharedBufferData = m_argumentInputSharedData.front();

			perPipelineData.modelOffset = static_cast<std::uint32_t>(
				sharedBufferData.offset / argumentStrideSize
			);
		}

		Buffer const* perPipelineBuffer = m_perPipelineSharedData.bufferData;

		memcpy(
			perPipelineBuffer->CPUHandle() + m_perPipelineSharedData.offset,
			&perPipelineData, perPipelineStride
		);
	}

	// Per Model Data
	{
		const auto pipelineIndex = static_cast<std::uint32_t>(
			m_perPipelineSharedData.offset / perPipelineStride
		);

		std::uint8_t* bufferStart = m_perModelSharedData.bufferData->CPUHandle();
		auto offset               = static_cast<size_t>(m_perModelSharedData.offset);

		for (size_t index = 0u; index < modelCount; ++index)
		{
			PerModelData perModelData
			{
				.pipelineIndex = pipelineIndex,
				.isVisible     = 1u
			};

			memcpy(bufferStart + offset, &perModelData, perModelStride);

			offset += perModelStride;
		}
	}
}

void PipelineModelsCSIndirect::AllocateBuffers(
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer
) {
	const size_t modelCount = GetModelCount();

	if (!modelCount)
		return;

	constexpr size_t argumentStrideSize = sizeof(IndirectArgument);
	constexpr size_t perModelStride     = sizeof(PerModelData);
	const auto perPipelineDataSize      = static_cast<UINT64>(sizeof(PerPipelineData));

	const auto argumentBufferSize = static_cast<UINT64>(argumentStrideSize * modelCount);
	const auto perModelDataSize   = static_cast<UINT64>(perModelStride * modelCount);

	// Input Arguments
	const size_t argumentInputBufferCount = std::size(argumentInputSharedBuffers);

	if (std::empty(m_argumentInputSharedData))
		m_argumentInputSharedData.resize(
			argumentInputBufferCount, SharedBufferData{ nullptr, 0u, 0u }
		);

	for (size_t index = 0u; index < argumentInputBufferCount; ++index)
	{
		SharedBufferData& argumentInputSharedData  = m_argumentInputSharedData[index];
		SharedBufferCPU& argumentInputSharedBuffer = argumentInputSharedBuffers[index];

		if (argumentInputSharedData.bufferData)
			argumentInputSharedBuffer.RelinquishMemory(argumentInputSharedData);

		argumentInputSharedData = argumentInputSharedBuffer.AllocateAndGetSharedData(
			argumentBufferSize, true
		);
	}

	// Per Pipeline Data.
	{
		if (m_perPipelineSharedData.bufferData)
			perPipelineSharedBuffer.RelinquishMemory(m_perPipelineSharedData);

		m_perPipelineSharedData = perPipelineSharedBuffer.AllocateAndGetSharedData(
			perPipelineDataSize, true
		);
	}

	// Per Model Data
	{
		if (m_perModelSharedData.bufferData)
			perModelSharedBuffer.RelinquishMemory(m_perModelSharedData);

		m_perModelSharedData = perModelSharedBuffer.AllocateAndGetSharedData(
			perModelDataSize, true
		);
	}
}

void PipelineModelsCSIndirect::Update(
	size_t frameIndex, const D3DMeshBundleVS& meshBundle,
	const std::vector<std::shared_ptr<Model>>& models
) const noexcept {
	const size_t modelCount = std::size(m_modelData);

	if (!modelCount)
		return;

	const SharedBufferData& argumentInputSharedData = m_argumentInputSharedData[frameIndex];

	std::uint8_t* argumentInputStart = argumentInputSharedData.bufferData->CPUHandle();
	std::uint8_t* perModelStart      = m_perModelSharedData.bufferData->CPUHandle();

	constexpr size_t argumentStride = sizeof(IndirectArgument);
	auto argumentOffset             = static_cast<size_t>(argumentInputSharedData.offset);

	constexpr size_t perModelStride = sizeof(PerModelData);
	auto perModelOffset             = static_cast<size_t>(m_perModelSharedData.offset);
	constexpr auto isVisibleOffset  = offsetof(PerModelData, isVisible);

	for (size_t index = 0u; index < modelCount; ++index)
	{
		const ModelData& modelData          = m_modelData[index];
		const std::shared_ptr<Model>& model = models[modelData.bundleIndex];

		const MeshTemporaryDetailsVS& meshDetailsVS = meshBundle.GetMeshDetails(model->GetMeshIndex());
		const D3D12_DRAW_INDEXED_ARGUMENTS meshArgs = PipelineModelsBase::GetDrawIndexedIndirectCommand(
			meshDetailsVS
		);

		IndirectArgument arguments
		{
			.modelIndex    = modelData.bufferIndex,
			.drawArguments = meshArgs
		};

		memcpy(argumentInputStart + argumentOffset, &arguments, argumentStride);

		argumentOffset += argumentStride;

		// Model Visiblity
		const auto visiblity = static_cast<std::uint32_t>(
			m_modelData.IsInUse(index) && model->IsVisible()
		);

		memcpy(perModelStart + perModelOffset + isVisibleOffset, &visiblity, sizeof(std::uint32_t));

		perModelOffset += perModelStride;
	}
}

void PipelineModelsCSIndirect::RelinquishMemory(
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer
) noexcept {
	// Input Buffers
	for (size_t index = 0u; index < std::size(m_argumentInputSharedData); ++index)
	{
		SharedBufferCPU& argumentSharedBuffer = argumentInputSharedBuffers[index];
		SharedBufferData& argumentSharedData  = m_argumentInputSharedData[index];

		if (argumentSharedData.bufferData)
		{
			argumentSharedBuffer.RelinquishMemory(argumentSharedData);

			argumentSharedData = SharedBufferData{ nullptr, 0u, 0u };
		}
	}

	// Per Pipeline Buffer
	if (m_perPipelineSharedData.bufferData)
	{
		perPipelineSharedBuffer.RelinquishMemory(m_perPipelineSharedData);

		m_perPipelineSharedData = SharedBufferData{ nullptr, 0u, 0u };
	}

	// Per Model Buffer
	if (m_perModelSharedData.bufferData)
	{
		perModelSharedBuffer.RelinquishMemory(m_perModelSharedData);

		m_perModelSharedData = SharedBufferData{ nullptr, 0u, 0u };
	}
}

// Pipeline Models VS Indirect
PipelineModelsVSIndirect::PipelineModelsVSIndirect()
	: m_modelCount{ 0u }, m_psoIndex{ 0u }, m_argumentOutputSharedData{}, m_counterSharedData{}
{}

void PipelineModelsVSIndirect::AllocateBuffers(
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers, UINT modelCount
) {
	constexpr size_t argStrideSize      = sizeof(PipelineModelsCSIndirect::IndirectArgument);
	const auto argumentOutputBufferSize = static_cast<UINT64>(modelCount * argStrideSize);

	m_modelCount                        = modelCount;

	if (!m_modelCount)
		return;

	// Argument Output
	{
		const size_t argumentOutputBufferCount = std::size(argumentOutputSharedBuffers);

		if (std::empty(m_argumentOutputSharedData))
			m_argumentOutputSharedData.resize(
				argumentOutputBufferCount, SharedBufferData{ nullptr, 0u, 0u }
			);

		for (size_t index = 0u; index < argumentOutputBufferCount; ++index)
		{
			SharedBufferData& argumentOutputSharedData           = m_argumentOutputSharedData[index];
			SharedBufferGPUWriteOnly& argumentOutputSharedBuffer = argumentOutputSharedBuffers[index];

			if (argumentOutputSharedData.bufferData)
				argumentOutputSharedBuffer.RelinquishMemory(argumentOutputSharedData);

			argumentOutputSharedData = argumentOutputSharedBuffer.AllocateAndGetSharedData(
				argumentOutputBufferSize
			);
		}
	}

	// Counter Buffer
	{
		const size_t counterBufferCount = std::size(counterSharedBuffers);

		if (std::empty(m_counterSharedData))
			m_counterSharedData.resize(
				counterBufferCount, SharedBufferData{ nullptr, 0u, 0u }
			);

		for (size_t index = 0u; index < counterBufferCount; ++index)
		{
			SharedBufferData& counterSharedData           = m_counterSharedData[index];
			SharedBufferGPUWriteOnly& counterSharedBuffer = counterSharedBuffers[index];

			if (counterSharedData.bufferData)
				counterSharedBuffer.RelinquishMemory(counterSharedData);

			counterSharedData = counterSharedBuffer.AllocateAndGetSharedData(
				s_counterBufferSize
			);
		}
	}
}

void PipelineModelsVSIndirect::Draw(
	size_t frameIndex, ID3D12CommandSignature* commandSignature, const D3DCommandList& graphicsList
) const noexcept {
	ID3D12GraphicsCommandList6* cmdList = graphicsList.Get();

	if (!m_modelCount)
		return;

	const SharedBufferData& argumentOutputSharedData = m_argumentOutputSharedData[frameIndex];
	const SharedBufferData& counterSharedData        = m_counterSharedData[frameIndex];

	cmdList->ExecuteIndirect(
		commandSignature, m_modelCount,
		argumentOutputSharedData.bufferData->Get(), argumentOutputSharedData.offset,
		counterSharedData.bufferData->Get(), counterSharedData.offset
	);
}

void PipelineModelsVSIndirect::RelinquishMemory(
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) noexcept {
	// Output Buffers
	for (size_t index = 0u; index < std::size(m_argumentOutputSharedData); ++index)
	{
		SharedBufferGPUWriteOnly& argumentSharedBuffer = argumentOutputSharedBuffers[index];
		SharedBufferData& argumentSharedData           = m_argumentOutputSharedData[index];

		if (argumentSharedData.bufferData)
		{
			argumentSharedBuffer.RelinquishMemory(argumentSharedData);

			argumentSharedData = SharedBufferData{ nullptr, 0u, 0u };
		}
	}

	// Counter Shared Buffer
	for (size_t index = 0u; index < std::size(m_counterSharedData); ++index)
	{
		SharedBufferGPUWriteOnly& counterSharedBuffer = counterSharedBuffers[index];
		SharedBufferData& counterSharedData           = m_counterSharedData[index];

		if (counterSharedData.bufferData)
		{
			counterSharedBuffer.RelinquishMemory(counterSharedData);

			counterSharedData = SharedBufferData{ nullptr, 0u, 0u };
		}
	}
}

// Model Bundle VS Individual
void ModelBundleVSIndividual::Draw(
	const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle,
	const PipelineManager<GraphicsPipeline_t>& pipelineManager
) const noexcept {
	meshBundle.Bind(graphicsList);

	const auto& models         = m_modelBundle->GetModels();

	const size_t pipelineCount = std::size(m_pipelines);

	for (size_t index = 0u; index < pipelineCount; ++index)
	{
		if (!m_pipelines.IsInUse(index))
			continue;

		const PipelineModelsVSIndividual& pipeline = m_pipelines[index];

		pipelineManager.BindPipeline(pipeline.GetPSOIndex(), graphicsList);

		pipeline.Draw(graphicsList, constantsRootIndex, meshBundle, models);
	}
}

void ModelBundleVSIndividual::DrawPipeline(
	size_t pipelineLocalIndex, const D3DCommandList& graphicsList, UINT constantsRootIndex,
	const D3DMeshBundleVS& meshBundle
) const noexcept {
	if (!m_pipelines.IsInUse(pipelineLocalIndex))
		return;

	meshBundle.Bind(graphicsList);

	const auto& models = m_modelBundle->GetModels();

	const PipelineModelsVSIndividual& pipeline = m_pipelines[pipelineLocalIndex];

	pipeline.Draw(graphicsList, constantsRootIndex, meshBundle, models);
}

// Model Bundle MS Individual
void ModelBundleMSIndividual::SetMeshBundleConstants(
	ID3D12GraphicsCommandList* graphicsList, UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle
) noexcept {
	constexpr UINT constBufferCount = D3DMeshBundleMS::GetConstantCount();

	const D3DMeshBundleMS::MeshBundleDetailsMS meshBundleDetailsMS = meshBundle.GetMeshBundleDetailsMS();

	graphicsList->SetGraphicsRoot32BitConstants(
		constantsRootIndex, constBufferCount, &meshBundleDetailsMS, 0u
	);
}

void ModelBundleMSIndividual::Draw(
	const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle,
	const PipelineManager<GraphicsPipeline_t>& pipelineManager
) const noexcept {
	SetMeshBundleConstants(graphicsList.Get(), constantsRootIndex, meshBundle);

	const auto& models         = m_modelBundle->GetModels();

	const size_t pipelineCount = std::size(m_pipelines);

	for (size_t index = 0u; index < pipelineCount; ++index)
	{
		if (!m_pipelines.IsInUse(index))
			continue;

		const PipelineModelsMSIndividual& pipeline = m_pipelines[index];

		pipelineManager.BindPipeline(pipeline.GetPSOIndex(), graphicsList);

		pipeline.Draw(graphicsList, constantsRootIndex, meshBundle, models);
	}
}

void ModelBundleMSIndividual::DrawPipeline(
	size_t pipelineLocalIndex, const D3DCommandList& graphicsList, UINT constantsRootIndex,
	const D3DMeshBundleMS& meshBundle
) const noexcept {
	if (!m_pipelines.IsInUse(pipelineLocalIndex))
		return;

	SetMeshBundleConstants(graphicsList.Get(), constantsRootIndex, meshBundle);

	const auto& models = m_modelBundle->GetModels();

	const PipelineModelsMSIndividual& pipeline = m_pipelines[pipelineLocalIndex];

	pipeline.Draw(graphicsList, constantsRootIndex, meshBundle, models);
}

// Model Bundle VS Indirect
std::uint32_t ModelBundleVSIndirect::AddPipeline(std::uint32_t pipelineIndex)
{
	const size_t pipelineLocalIndex = _addPipeline(pipelineIndex);

	if (pipelineLocalIndex >= std::size(m_vsPipelines))
		m_vsPipelines.emplace_back(PipelineModelsVSIndirect{});

	assert(
		pipelineLocalIndex + 1u == std::size(m_vsPipelines)
		&& "CS Pipeline is supposed to not have any extra allocations and only allocate a single pipeline."
	);

	PipelineModelsVSIndirect& vsPipeline = m_vsPipelines[pipelineLocalIndex];

	vsPipeline.SetPSOIndex(pipelineIndex);

	return static_cast<std::uint32_t>(pipelineLocalIndex);
}

void ModelBundleVSIndirect::CleanupData(
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) noexcept {
	const size_t pipelineCount = std::size(m_pipelines);

	for (size_t index = 0u; index < pipelineCount; ++index)
		RemovePipeline(
			index, argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
			argumentOutputSharedBuffers, counterSharedBuffers
		);

	operator=(ModelBundleVSIndirect{});
}

void ModelBundleVSIndirect::RemovePipeline(
	size_t pipelineLocalIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) noexcept {
	// CS
	PipelineModelsCSIndirect& csPipeline = m_pipelines[pipelineLocalIndex];

	csPipeline.ResetCullingData();

	csPipeline.RelinquishMemory(
		argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
	);
	// The cleanup will be done in _removePipeline

	// VS
	PipelineModelsVSIndirect& vsPipeline = m_vsPipelines[pipelineLocalIndex];

	vsPipeline.RelinquishMemory(argumentOutputSharedBuffers, counterSharedBuffers);
	vsPipeline.CleanupData();

	_removePipeline(pipelineLocalIndex);
}

void ModelBundleVSIndirect::AddModel(
	std::uint32_t pipelineIndex, std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	const std::uint32_t bufferIndex = m_modelBufferIndices[modelIndex];

	_addModels(
		pipelineIndex, modelBundleIndex, &modelIndex, &bufferIndex, 1u,
		argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
		argumentOutputSharedBuffers, counterSharedBuffers
	);
}

void ModelBundleVSIndirect::AddModels(
	std::uint32_t pipelineIndex, std::uint32_t modelBundleIndex,
	const std::vector<std::uint32_t>& modelIndices,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	const size_t modelCount = std::size(modelIndices);

	std::vector<std::uint32_t> bufferIndices(modelCount);

	for (size_t index = 0u; index < modelCount; ++index)
		bufferIndices[index] = m_modelBufferIndices[modelIndices[index]];

	_addModels(
		pipelineIndex, modelBundleIndex, std::data(modelIndices), std::data(bufferIndices),
		modelCount, argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
		argumentOutputSharedBuffers, counterSharedBuffers
	);
}

void ModelBundleVSIndirect::_moveModel(
	std::uint32_t modelIndex, std::uint32_t modelBundleIndex,
	std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	const MoveModelCommonData moveData = _moveModelCommon(modelIndex, oldPipelineIndex, newPipelineIndex);

	std::uint32_t newPipelineLocalIndex            = moveData.newLocalPipelineIndex;
	const PipelineModelsBase::ModelData& modelData = moveData.removedModelData;

	// I feel like the new pipeline being not there already should be fine and we should add it.
	if (newPipelineLocalIndex == std::numeric_limits<std::uint32_t>::max())
		newPipelineLocalIndex = AddPipeline(newPipelineIndex);

	if (modelData.bundleIndex != std::numeric_limits<std::uint32_t>::max())
		_addModelsToPipeline(
			newPipelineLocalIndex, modelBundleIndex,
			&modelData.bundleIndex, &modelData.bufferIndex, 1u,
			argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
			argumentOutputSharedBuffers, counterSharedBuffers
		);
}

void ModelBundleVSIndirect::MoveModel(
	std::uint32_t modelIndex, std::uint32_t modelBundleIndex,
	std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	_moveModel(
		modelIndex, modelBundleIndex, oldPipelineIndex, newPipelineIndex,
		argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
		argumentOutputSharedBuffers, counterSharedBuffers
	);
}

size_t ModelBundleVSIndirect::GetLocalPipelineIndex(std::uint32_t pipelineIndex) noexcept
{
	auto pipelineLocalIndex = std::numeric_limits<size_t>::max();

	std::optional<size_t> oPipelineLocalIndex = FindPipeline(pipelineIndex);

	if (oPipelineLocalIndex)
		pipelineLocalIndex = oPipelineLocalIndex.value();
	else
		pipelineLocalIndex = AddPipeline(pipelineIndex);

	return pipelineLocalIndex;
}

size_t ModelBundleVSIndirect::FindAddableStartIndex(
	size_t pipelineLocalIndex, size_t modelCount
) const noexcept {
	auto addableStartIndex   = std::numeric_limits<size_t>::max();
	size_t totalAddableModel = 0u;

	for (size_t index = pipelineLocalIndex; index > 0u; --index)
	{
		const PipelineModelsCSIndirect& pipeline = m_pipelines[index];

		const size_t currentAddableModel         = pipeline.GetAddableModelCount();

		if (currentAddableModel)
		{
			totalAddableModel += currentAddableModel;

			if (totalAddableModel >= modelCount)
			{
				addableStartIndex = index;

				break;
			}
		}
	}

	if (addableStartIndex == std::numeric_limits<size_t>::max())
	{
		const size_t index = 0u;

		const PipelineModelsCSIndirect& pipeline = m_pipelines[index];

		const size_t currentAddableModel         = pipeline.GetAddableModelCount();

		if (currentAddableModel)
		{
			totalAddableModel += currentAddableModel;

			if (totalAddableModel >= modelCount)
				addableStartIndex = index;
		}
	}

	return addableStartIndex;
}

void ModelBundleVSIndirect::ResizePreviousPipelines(
	size_t addableStartIndex, size_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	for (size_t index = addableStartIndex; index < pipelineLocalIndex; ++index)
	{
		PipelineModelsCSIndirect& csPipeline = m_pipelines[index];
		PipelineModelsVSIndirect& vsPipeline = m_vsPipelines[index];

		const std::vector<PipelineModelsCSIndirect::IndexLink>& indexLinks
			= csPipeline.RemoveInactiveModels();

		LocalIndexMap_t& pipelineMap = m_localModelIndexMap[index];

		for (const PipelineModelsCSIndirect::IndexLink& indexLink : indexLinks)
			pipelineMap[indexLink.bundleIndex] = indexLink.localIndex;

		csPipeline.AllocateBuffers(
			argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
		);

		csPipeline.UpdateNonPerFrameData(modelBundleIndex);

		const auto newModelCount = static_cast<std::uint32_t>(csPipeline.GetModelCount());

		vsPipeline.AllocateBuffers(argumentOutputSharedBuffers, counterSharedBuffers, newModelCount);
	}
}

void ModelBundleVSIndirect::RecreateFollowingPipelines(
	size_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	const size_t pipelineCount = std::size(m_pipelines);

	for (size_t index = pipelineLocalIndex; index < pipelineCount; ++index)
	{
		PipelineModelsCSIndirect& csPipeline = m_pipelines[index];
		PipelineModelsVSIndirect& vsPipeline = m_vsPipelines[index];

		// CS
		// Must free the memory first, otherwise the buffers of the current pipeline
		// will be created at the end.
		csPipeline.RelinquishMemory(
			argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
		);

		csPipeline.AllocateBuffers(
			argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
		);

		csPipeline.UpdateNonPerFrameData(modelBundleIndex);

		// VS
		vsPipeline.RelinquishMemory(argumentOutputSharedBuffers, counterSharedBuffers);

		const auto newModelCount = static_cast<std::uint32_t>(csPipeline.GetModelCount());

		vsPipeline.AllocateBuffers(argumentOutputSharedBuffers, counterSharedBuffers, newModelCount);
	}
}

void ModelBundleVSIndirect::_addModels(
	std::uint32_t pipelineIndex, std::uint32_t modelBundleIndex,
	std::uint32_t const* modelIndices, std::uint32_t const* bufferIndices,
	size_t modelCount, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	const auto pipelineLocalIndex = static_cast<std::uint32_t>(GetLocalPipelineIndex(pipelineIndex));

	_addModelsToPipeline(
		pipelineLocalIndex, modelBundleIndex, modelIndices, bufferIndices, modelCount,
		argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
		argumentOutputSharedBuffers, counterSharedBuffers
	);
}

void ModelBundleVSIndirect::_addModelsToPipeline(
	std::uint32_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
	std::uint32_t const* modelIndices, std::uint32_t const* bufferIndices,
	size_t modelCount, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	PipelineModelsCSIndirect& pipeline = m_pipelines[pipelineLocalIndex];

	// This count must be kept before adding the model data.
	const size_t addableModelCount     = pipeline.GetAddableModelCount();

	// Add the model data first.
	for (size_t index = 0u; index < modelCount; ++index)
	{
		const std::uint32_t modelIndex  = modelIndices[index];
		const std::uint32_t bufferIndex = bufferIndices[index];

		const std::uint32_t localModelIndex = pipeline.AddModel(modelIndex, bufferIndex);

		LocalIndexMap_t& pipelineMap        = m_localModelIndexMap[pipelineLocalIndex];

		pipelineMap.insert_or_assign(modelIndex, localModelIndex);
	}

	const auto pipelineCount = static_cast<std::uint32_t>(std::size(m_pipelines));

	// We don't have to mess with any other pipelines if ours is the last pipeline.
	if (pipelineLocalIndex + 1u == pipelineCount)
	{
		if (modelCount > addableModelCount)
		{
			pipeline.AllocateBuffers(
				argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
			);

			const auto newPipelineModelCount = static_cast<std::uint32_t>(pipeline.GetModelCount());

			m_vsPipelines[pipelineLocalIndex].AllocateBuffers(
				argumentOutputSharedBuffers, counterSharedBuffers, newPipelineModelCount
			);
		}

		pipeline.UpdateNonPerFrameData(modelBundleIndex);
	}
	else
	{
		const size_t addableStartIndex = FindAddableStartIndex(pipelineLocalIndex, modelCount);

		// If we have enough free space in the previous pipelines, we should be able to
		// resize them and fit this one. This will be useful for moving and it wouldn't
		// require any actual new buffer allocation.
		if (addableStartIndex != std::numeric_limits<size_t>::max())
			ResizePreviousPipelines(
				addableStartIndex, pipelineLocalIndex, modelBundleIndex,
				argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
				argumentOutputSharedBuffers, counterSharedBuffers
			);
		// Otherwise we will have to increase the buffer size and also recreate all the
		// buffers of the following pipelines.
		else
			RecreateFollowingPipelines(
				pipelineLocalIndex, modelBundleIndex,
				argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
				argumentOutputSharedBuffers, counterSharedBuffers
			);
	}
}

void ModelBundleVSIndirect::Update(size_t frameIndex, const D3DMeshBundleVS& meshBundle) const noexcept
{
	const auto& models         = m_modelBundle->GetModels();

	const size_t pipelineCount = std::size(m_pipelines);

	for (size_t index = 0u; index < pipelineCount; ++index)
	{
		if (!m_pipelines.IsInUse(index))
			continue;

		const PipelineModelsCSIndirect& pipeline = m_pipelines[index];

		pipeline.Update(frameIndex, meshBundle, models);
	}
}

void ModelBundleVSIndirect::UpdatePipeline(
	size_t pipelineLocalIndex, size_t frameIndex, const D3DMeshBundleVS& meshBundle
) const noexcept {
	const auto& models = m_modelBundle->GetModels();

	if (!m_pipelines.IsInUse(pipelineLocalIndex))
		return;

	const PipelineModelsCSIndirect& pipeline = m_pipelines[pipelineLocalIndex];

	pipeline.Update(frameIndex, meshBundle, models);
}

void ModelBundleVSIndirect::Draw(
	size_t frameIndex, const D3DCommandList& graphicsList, ID3D12CommandSignature* commandSignature,
	const D3DMeshBundleVS& meshBundle, const PipelineManager<GraphicsPipeline_t>& pipelineManager
) const noexcept {
	meshBundle.Bind(graphicsList);

	const size_t pipelineCount = std::size(m_pipelines);

	for (size_t index = 0u; index < pipelineCount; ++index)
	{
		if (!m_pipelines.IsInUse(index))
			continue;

		const PipelineModelsVSIndirect& pipeline = m_vsPipelines[index];

		pipelineManager.BindPipeline(pipeline.GetPSOIndex(), graphicsList);

		pipeline.Draw(frameIndex, commandSignature, graphicsList);
	}
}

void ModelBundleVSIndirect::DrawPipeline(
	size_t pipelineLocalIndex, size_t frameIndex, const D3DCommandList& graphicsList,
	ID3D12CommandSignature* commandSignature, const D3DMeshBundleVS& meshBundle
) const noexcept {
	if (!m_pipelines.IsInUse(pipelineLocalIndex))
		return;

	meshBundle.Bind(graphicsList);

	const PipelineModelsVSIndirect& pipeline = m_vsPipelines[pipelineLocalIndex];

	pipeline.Draw(frameIndex, commandSignature, graphicsList);
}
