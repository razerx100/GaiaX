#include <D3DModelBundle.hpp>
#include <VectorToSharedPtr.hpp>

// Model Bundle
D3D12_DRAW_INDEXED_ARGUMENTS ModelBundleBase::GetDrawIndexedIndirectCommand(
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

// Model Bundle VS Individual
void ModelBundleVSIndividual::SetModelBundle(
	std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBufferIndices = std::move(modelBufferIndices);
	m_modelBundle        = std::move(bundle);
}

void ModelBundleVSIndividual::Draw(
	const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleVS& meshBundle
) const noexcept {
	ID3D12GraphicsCommandList* cmdList = graphicsList.Get();
	const auto& models                 = m_modelBundle->GetModels();

	for (size_t index = 0; index < std::size(models); ++index)
	{
		const std::shared_ptr<Model>& model = models[index];

		if (!model->IsVisible())
			continue;

		constexpr UINT pushConstantCount = GetConstantCount();

		cmdList->SetGraphicsRoot32BitConstants(
			constantsRootIndex, pushConstantCount, &m_modelBufferIndices[index], 0u
		);

		const D3D12_DRAW_INDEXED_ARGUMENTS meshArgs = GetDrawIndexedIndirectCommand(
			meshBundle.GetMeshDetails(model->GetMeshIndex())
		);

		cmdList->DrawIndexedInstanced(
			meshArgs.IndexCountPerInstance, meshArgs.InstanceCount, meshArgs.StartIndexLocation,
			meshArgs.BaseVertexLocation, meshArgs.StartInstanceLocation
		);
	}
}

// Model Bundle MS
void ModelBundleMSIndividual::SetModelBundle(
	std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBundle        = std::move(bundle);
	m_modelBufferIndices = std::move(modelBufferIndices);
}

void ModelBundleMSIndividual::Draw(
	const D3DCommandList& graphicsList, UINT constantsRootIndex, const D3DMeshBundleMS& meshBundle
) const noexcept {
	ID3D12GraphicsCommandList6* cmdList = graphicsList.Get();
	const auto& models                  = m_modelBundle->GetModels();

	for (size_t index = 0u; index < std::size(models); ++index)
	{
		const std::shared_ptr<Model>& model = models[index];

		if (!model->IsVisible())
			continue;

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
			.modelBufferIndex = m_modelBufferIndices[index]
		};

		cmdList->SetGraphicsRoot32BitConstants(
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

		cmdList->DispatchMesh(amplficationGroupCount, 1u, 1u);
		// It might be worth checking if we are reaching the Group Count Limit and if needed
		// launch more Groups. Could achieve that by passing a GroupLaunch index.
	}
}

// Model Bundle VS Indirect
ModelBundleVSIndirect::ModelBundleVSIndirect()
	: ModelBundleBase{}, m_modelCount{ 0u }, m_modelBundle{}, m_argumentOutputSharedData{},
	m_counterSharedData{}, m_bundleID{ 0u }
{}

void ModelBundleVSIndirect::SetModelBundle(std::shared_ptr<ModelBundle> bundle) noexcept
{
	m_modelBundle = std::move(bundle);
}

void ModelBundleVSIndirect::CreateBuffers(
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers, UINT modelCount
) {
	constexpr size_t argStrideSize      = sizeof(ModelBundleCSIndirect::IndirectArgument);
	const auto argumentOutputBufferSize = static_cast<UINT64>(modelCount * argStrideSize);
	m_modelCount                        = modelCount;

	{
		const size_t argumentOutputBufferCount = std::size(argumentOutputSharedBuffers);
		m_argumentOutputSharedData.resize(argumentOutputBufferCount);

		for (size_t index = 0u; index < argumentOutputBufferCount; ++index)
		{
			SharedBufferData& argumentOutputSharedData = m_argumentOutputSharedData[index];

			argumentOutputSharedData = argumentOutputSharedBuffers[index].AllocateAndGetSharedData(
				argumentOutputBufferSize
			);
		}
	}

	{
		const size_t counterBufferCount = std::size(counterSharedBuffers);
		m_counterSharedData.resize(counterBufferCount);

		for (size_t index = 0u; index < counterBufferCount; ++index)
			m_counterSharedData[index] = counterSharedBuffers[index].AllocateAndGetSharedData(
				s_counterBufferSize
			);
	}
}

void ModelBundleVSIndirect::Draw(
	size_t frameIndex, ID3D12CommandSignature* commandSignature, const D3DCommandList& graphicsList
) const noexcept {
	ID3D12GraphicsCommandList6* cmdList = graphicsList.Get();

	const SharedBufferData& argumentOutputSharedData = m_argumentOutputSharedData[frameIndex];
	const SharedBufferData& counterSharedData        = m_counterSharedData[frameIndex];

	cmdList->ExecuteIndirect(
		commandSignature, m_modelCount,
		argumentOutputSharedData.bufferData->Get(), argumentOutputSharedData.offset,
		counterSharedData.bufferData->Get(), counterSharedData.offset
	);
}

// Model Bundle CS Indirect
ModelBundleCSIndirect::ModelBundleCSIndirect()
	: m_perModelSharedData{ nullptr, 0u, 0u }, m_cullingSharedData{ nullptr, 0u, 0u },
	m_argumentInputSharedData{}, m_modelBundle{}, m_modelIndices{}
{}

void ModelBundleCSIndirect::SetModelBundle(
	std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelIndices
) noexcept {
	m_modelBundle  = std::move(bundle);
	m_modelIndices = std::move(modelIndices);
}

void ModelBundleCSIndirect::ResetCullingData() const noexcept
{
	// Before destroying an object the command count needs to be set to 0,
	// so the model isn't processed in the compute shader anymore.
	std::uint32_t commandCount  = 0u;

	Buffer const* cullingBuffer = m_cullingSharedData.bufferData;

	memcpy(
		cullingBuffer->CPUHandle() + m_cullingSharedData.offset,
		&commandCount, sizeof(std::uint32_t)
	);
}

void ModelBundleCSIndirect::CreateBuffers(
	std::vector<SharedBufferCPU>& argumentInputSharedBuffer,
	SharedBufferCPU& cullingSharedBuffer, SharedBufferCPU& perModelDataSharedBuffer
) {
	constexpr size_t argumentStrideSize = sizeof(IndirectArgument);
	constexpr size_t perModelStride     = sizeof(PerModelData);

	const auto argumentCount      = static_cast<std::uint32_t>(std::size(m_modelBundle->GetModels()));
	const auto argumentBufferSize = static_cast<UINT64>(argumentStrideSize * argumentCount);
	const auto cullingDataSize    = static_cast<UINT64>(sizeof(CullingData));
	const auto perModelDataSize   = static_cast<UINT64>(perModelStride * argumentCount);

	{
		const size_t argumentInputBufferCount = std::size(argumentInputSharedBuffer);
		m_argumentInputSharedData.resize(argumentInputBufferCount);

		for (size_t index = 0u; index < argumentInputBufferCount; ++index)
			m_argumentInputSharedData[index] = argumentInputSharedBuffer[index].AllocateAndGetSharedData(
				argumentBufferSize
			);
	}

	// Set the culling data.
	{
		m_cullingSharedData = cullingSharedBuffer.AllocateAndGetSharedData(
			cullingDataSize, true
		);

		CullingData cullingData
		{
			.commandCount  = argumentCount,
			.commandOffset = 0u
		};

		if (!std::empty(m_argumentInputSharedData))
		{
			const SharedBufferData& sharedBufferData = m_argumentInputSharedData.front();

			// The offset on each sharedBuffer should be the same.
			cullingData.commandOffset = static_cast<std::uint32_t>(
				sharedBufferData.offset / argumentStrideSize
			);
		}

		Buffer const* cullingBuffer = m_cullingSharedData.bufferData;

		memcpy(
			cullingBuffer->CPUHandle() + m_cullingSharedData.offset,
			&cullingData, cullingDataSize
		);
	}

	{
		const auto modelBundleIndex = GetModelBundleIndex();

		// Each thread will process a single model independently. And since we are trying to
		// cull all of the models across all of the bundles with a single call to dispatch, we can't
		// set the index as constantData per bundle. So, we will be giving each model the index
		// of its bundle so each thread can work independently.
		m_perModelSharedData = perModelDataSharedBuffer.AllocateAndGetSharedData(
			perModelDataSize, true
		);

		std::uint8_t* bufferStart = m_perModelSharedData.bufferData->CPUHandle();
		auto offset               = static_cast<size_t>(m_perModelSharedData.offset);

		const size_t modelCount   = std::size(m_modelIndices);

		for (size_t index = 0u; index < modelCount; ++index)
		{
			PerModelData perModelData
			{
				.bundleIndex = modelBundleIndex,
				.isVisible   = 1u
			};

			memcpy(bufferStart + offset, &perModelData, perModelStride);

			offset += perModelStride;
		}
	}
}

void ModelBundleCSIndirect::Update(size_t bufferIndex, const D3DMeshBundleVS& meshBundle) const noexcept
{
	const SharedBufferData& argumentInputSharedData = m_argumentInputSharedData[bufferIndex];

	std::uint8_t* argumentInputStart = argumentInputSharedData.bufferData->CPUHandle();
	std::uint8_t* perModelStart      = m_perModelSharedData.bufferData->CPUHandle();

	constexpr size_t argumentStride = sizeof(IndirectArgument);
	auto argumentOffset             = static_cast<size_t>(argumentInputSharedData.offset);

	constexpr size_t perModelStride = sizeof(PerModelData);
	auto perModelOffset             = static_cast<size_t>(m_perModelSharedData.offset);
	constexpr auto isVisibleOffset  = offsetof(PerModelData, isVisible);

	const auto& models              = m_modelBundle->GetModels();
	const size_t modelCount         = std::size(models);

	for (size_t index = 0u; index < modelCount; ++index)
	{
		const std::shared_ptr<Model>& model = models[index];
		const UINT modelIndex               = m_modelIndices[index];

		IndirectArgument arguments
		{
			.modelIndex    = modelIndex,
			.drawArguments = ModelBundleBase::GetDrawIndexedIndirectCommand(
				meshBundle.GetMeshDetails(model->GetMeshIndex())
			)
		};

		memcpy(argumentInputStart + argumentOffset, &arguments, argumentStride);

		argumentOffset += argumentStride;

		// Model Visiblity
		const auto visiblity = static_cast<std::uint32_t>(model->IsVisible());

		memcpy(perModelStart + perModelOffset + isVisibleOffset, &visiblity, sizeof(std::uint32_t));

		perModelOffset += perModelStride;
	}
}
