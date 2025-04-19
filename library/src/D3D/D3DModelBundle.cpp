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

void PipelineModelsBase::SetPipelineModelBundle(
	std::shared_ptr<PipelineModelBundle> pipelineBundle
) noexcept {
	m_pipelineBundle = std::move(pipelineBundle);
}

// Pipeline Models VS Individual
void PipelineModelsVSIndividual::DrawModel(
	const std::shared_ptr<Model>& model, ID3D12GraphicsCommandList* graphicsList,
	UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle
) const noexcept {
	if (!model->IsVisible())
		return;

	constexpr UINT pushConstantCount = GetConstantCount();

	const std::uint32_t indexInBuffer = model->GetModelIndexInBuffer();

	graphicsList->SetGraphicsRoot32BitConstants(
		constantsRootIndex, pushConstantCount, &indexInBuffer, 0u
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
	const auto& pipelineModelIndices = m_pipelineBundle->GetModelIndicesInBundle();

	ID3D12GraphicsCommandList* cmdList = graphicsList.Get();

	for (std::uint32_t modelIndexInBundle : pipelineModelIndices)
		DrawModel(models[modelIndexInBundle], cmdList, constantsRootIndex, meshBundle);
}

// Pipeline Models MS Individual
void PipelineModelsMSIndividual::DrawModel(
	const std::shared_ptr<Model>& model, ID3D12GraphicsCommandList6* graphicsList,
	UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle
) const noexcept {
	if (!model->IsVisible())
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
		.modelBufferIndex = model->GetModelIndexInBuffer()
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
	const auto& pipelineModelIndices = m_pipelineBundle->GetModelIndicesInBundle();

	ID3D12GraphicsCommandList6* cmdList = graphicsList.Get();

	for (std::uint32_t modelIndexInBundle : pipelineModelIndices)
		DrawModel(models[modelIndexInBundle], cmdList, constantsRootIndex, meshBundle);
}

// Pipeline Models CS Indirect
PipelineModelsCSIndirect::PipelineModelsCSIndirect()
	: PipelineModelsBase{}, m_perPipelineSharedData{ nullptr, 0u, 0u },
	m_perModelSharedData{ nullptr, 0u, 0u }, m_argumentInputSharedData{}
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

size_t PipelineModelsCSIndirect::GetAddableModelCount() const noexcept
{
	constexpr size_t perModelStride = sizeof(PerModelData);

	const size_t currentModelCount  = std::size(m_pipelineBundle->GetModelIndicesInBundle());

	const auto currentPerModelBufferSize = static_cast<size_t>(m_perModelSharedData.size);

	const size_t allocatedModelCount     = currentPerModelBufferSize / perModelStride;

	return allocatedModelCount > currentModelCount ? allocatedModelCount - currentModelCount : 0u;
}

size_t PipelineModelsCSIndirect::GetNewModelCount() const noexcept
{
	constexpr size_t perModelStride = sizeof(PerModelData);

	const size_t currentModelCount  = std::size(m_pipelineBundle->GetModelIndicesInBundle());

	const auto currentPerModelBufferSize = static_cast<size_t>(m_perModelSharedData.size);

	const size_t allocatedModelCount     = currentPerModelBufferSize / perModelStride;

	return allocatedModelCount < currentModelCount ?  currentModelCount - allocatedModelCount : 0u;
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
				.modelFlags    = static_cast<std::uint32_t>(ModelFlag::Visibility)
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
	size_t frameIndex, const D3DMeshBundleVS& meshBundle, bool skipCulling,
	const std::vector<std::shared_ptr<Model>>& models
) const noexcept {
	const auto& pipelineModelIndices = m_pipelineBundle->GetModelIndicesInBundle();

	const size_t modelCount = std::size(pipelineModelIndices);

	if (!modelCount)
		return;

	const SharedBufferData& argumentInputSharedData = m_argumentInputSharedData[frameIndex];

	std::uint8_t* argumentInputStart = argumentInputSharedData.bufferData->CPUHandle();
	std::uint8_t* perModelStart      = m_perModelSharedData.bufferData->CPUHandle();

	constexpr size_t argumentStride = sizeof(IndirectArgument);
	auto argumentOffset             = static_cast<size_t>(argumentInputSharedData.offset);

	constexpr size_t perModelStride = sizeof(PerModelData);
	auto perModelOffset             = static_cast<size_t>(m_perModelSharedData.offset);
	constexpr auto modelFlagsOffset = offsetof(PerModelData, modelFlags);

	std::uint32_t skipCullingFlag
		= skipCulling ? static_cast<std::uint32_t>(ModelFlag::SkipCulling) : 0u;

	for (std::uint32_t modelIndexInBundle : pipelineModelIndices)
	{
		const std::shared_ptr<Model>& model = models[modelIndexInBundle];

		const MeshTemporaryDetailsVS& meshDetailsVS
			= meshBundle.GetMeshDetails(model->GetMeshIndex());
		const D3D12_DRAW_INDEXED_ARGUMENTS meshArgs
			= PipelineModelsBase::GetDrawIndexedIndirectCommand(meshDetailsVS);

		IndirectArgument arguments
		{
			.modelIndex    = model->GetModelIndexInBuffer(),
			.drawArguments = meshArgs
		};

		memcpy(argumentInputStart + argumentOffset, &arguments, argumentStride);

		argumentOffset += argumentStride;

		// Model Flags
		std::uint32_t modelFlags = skipCullingFlag;

		if (model->IsVisible())
			modelFlags |= static_cast<std::uint32_t>(ModelFlag::Visibility);

		memcpy(
			perModelStart + perModelOffset + modelFlagsOffset, &modelFlags, sizeof(std::uint32_t)
		);

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
	: m_argumentOutputSharedData{}, m_counterSharedData{}, m_modelCount{ 0u }
{}

void PipelineModelsVSIndirect::AllocateBuffers(
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	constexpr size_t argStrideSize      = sizeof(PipelineModelsCSIndirect::IndirectArgument);
	const auto argumentOutputBufferSize = static_cast<UINT64>(m_modelCount * argStrideSize);

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
			SharedBufferData& argumentOutputSharedData = m_argumentOutputSharedData[index];
			SharedBufferGPUWriteOnly& argumentOutputSharedBuffer
				= argumentOutputSharedBuffers[index];

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
	size_t frameIndex, ID3D12CommandSignature* commandSignature,
	const D3DCommandList& graphicsList
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
	ID3D12GraphicsCommandList* graphicsList, UINT constantsRootIndex,
	const D3DMeshBundleMS& meshBundle
) noexcept {
	constexpr UINT constBufferCount = D3DMeshBundleMS::GetConstantCount();

	const D3DMeshBundleMS::MeshBundleDetailsMS meshBundleDetailsMS
		= meshBundle.GetMeshBundleDetailsMS();

	graphicsList->SetGraphicsRoot32BitConstants(
		constantsRootIndex, constBufferCount, &meshBundleDetailsMS, 0u
	);
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
void ModelBundleVSIndirect::AddNewPipelinesFromBundle(
	std::uint32_t modelBundleIndex, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	const std::vector<std::shared_ptr<PipelineModelBundle>>& pipelines
		= m_modelBundle->GetPipelineBundles();

	const size_t pipelinesInBundle    = std::size(pipelines);
	const size_t currentPipelineCount = std::size(m_pipelines);

	for (size_t index = currentPipelineCount; index < pipelinesInBundle; ++index)
	{
		const size_t pipelineLocalIndex = _addPipeline(pipelines[index]);

		// Since the cs pipeline is a Reusable vector, it can reuse old pipelines
		// in that case we don't need to create a new vs pipeline.
		if (pipelineLocalIndex >= std::size(m_vsPipelines))
			m_vsPipelines.emplace_back(PipelineModelsVSIndirect{});

		SetupPipelineBuffers(
			static_cast<std::uint32_t>(pipelineLocalIndex), modelBundleIndex,
			argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
			argumentOutputSharedBuffers, counterSharedBuffers
		);
	}
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

size_t ModelBundleVSIndirect::GetLocalPipelineIndex(std::uint32_t pipelineIndex) noexcept
{
	auto pipelineLocalIndex = std::numeric_limits<size_t>::max();

	std::optional<size_t> oPipelineLocalIndex = FindPipeline(pipelineIndex);

	assert(oPipelineLocalIndex && "Local pipeline doesn't exist.");

	if (oPipelineLocalIndex)
		pipelineLocalIndex = oPipelineLocalIndex.value();

	return pipelineLocalIndex;
}

void ModelBundleVSIndirect::ReconfigureModels(
	std::uint32_t modelBundleIndex, std::uint32_t decreasedModelsPipelineIndex,
	std::uint32_t increasedModelsPipelineIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	auto decreasedPipelineLocalIndex = std::numeric_limits<size_t>::max();
	auto increasedPipelineLocalIndex = std::numeric_limits<std::uint32_t>::max();

	// Find the pipelines
	const size_t pipelineCount = std::size(m_pipelines);

	for (size_t index = 0u; index < pipelineCount; ++index)
	{
		const PipelineModelsCSIndirect& pipeline = m_pipelines[index];

		if (pipeline.GetPSOIndex() == decreasedModelsPipelineIndex)
			decreasedPipelineLocalIndex = index;

		if (pipeline.GetPSOIndex() == increasedModelsPipelineIndex)
			increasedPipelineLocalIndex = static_cast<std::uint32_t>(index);

		const bool bothFound
			= decreasedPipelineLocalIndex != std::numeric_limits<size_t>::max() &&
			increasedPipelineLocalIndex != std::numeric_limits<std::uint32_t>::max();

		if (bothFound)
			break;
	}

	// Need to update the model count on the decreased pipeline. So, the Compute Shader
	// doesn't process the moved model and the Vertex shader doesn't draw it.
	PipelineModelsCSIndirect& csPipeline = m_pipelines[decreasedPipelineLocalIndex];

	csPipeline.UpdateNonPerFrameData(modelBundleIndex);

	m_vsPipelines[decreasedPipelineLocalIndex].SetModelCount(csPipeline.GetModelCount());

	SetupPipelineBuffers(
		increasedPipelineLocalIndex, modelBundleIndex, argumentInputSharedBuffers,
		perPipelineSharedBuffer, perModelDataCSBuffer, argumentOutputSharedBuffers,
		counterSharedBuffers
	);
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

		csPipeline.AllocateBuffers(
			argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
		);

		csPipeline.UpdateNonPerFrameData(modelBundleIndex);

		vsPipeline.SetModelCount(csPipeline.GetModelCount());

		vsPipeline.AllocateBuffers(argumentOutputSharedBuffers, counterSharedBuffers);
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

		vsPipeline.SetModelCount(csPipeline.GetModelCount());

		vsPipeline.AllocateBuffers(argumentOutputSharedBuffers, counterSharedBuffers);
	}
}

void ModelBundleVSIndirect::SetupPipelineBuffers(
	std::uint32_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers
) {
	PipelineModelsCSIndirect& pipeline = m_pipelines[pipelineLocalIndex];

	const auto pipelineCount = static_cast<std::uint32_t>(std::size(m_pipelines));

	// We don't have to mess with any other pipelines if ours is the last pipeline.
	if (pipelineLocalIndex + 1u == pipelineCount)
	{
		pipeline.AllocateBuffers(
			argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
		);

		pipeline.UpdateNonPerFrameData(modelBundleIndex);

		PipelineModelsVSIndirect& vsPipeline = m_vsPipelines[pipelineLocalIndex];

		vsPipeline.SetModelCount(pipeline.GetModelCount());

		vsPipeline.AllocateBuffers(argumentOutputSharedBuffers, counterSharedBuffers);
	}
	else
	{
		const size_t addableStartIndex = FindAddableStartIndex(
			pipelineLocalIndex, pipeline.GetNewModelCount()
		);

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

void ModelBundleVSIndirect::UpdatePipeline(
	size_t pipelineLocalIndex, size_t frameIndex, const D3DMeshBundleVS& meshBundle,
	bool skipCulling
) const noexcept {
	const auto& models = m_modelBundle->GetModels();

	if (!m_pipelines.IsInUse(pipelineLocalIndex))
		return;

	const PipelineModelsCSIndirect& pipeline = m_pipelines[pipelineLocalIndex];

	pipeline.Update(frameIndex, meshBundle, skipCulling, models);
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
