#include <ModelManager.hpp>
#include <VectorToSharedPtr.hpp>
#include <D3DRootSignatureDynamic.hpp>

// Model Bundle
D3D12_DRAW_INDEXED_ARGUMENTS ModelBundle::GetDrawIndexedIndirectCommand(
	const std::shared_ptr<ModelVS>& model
) noexcept {
	MeshDetailsVS meshDetails = model->GetMeshDetailsVS();

	const D3D12_DRAW_INDEXED_ARGUMENTS indirectCommand
	{
		.IndexCountPerInstance = meshDetails.indexCount,
		.InstanceCount         = 1u,
		.StartIndexLocation    = meshDetails.indexOffset,
		.BaseVertexLocation    = 0,
		.StartInstanceLocation = 0u
	};

	return indirectCommand;
}

// Model Bundle VS Individual
void ModelBundleVSIndividual::SetModelBundle(
	std::shared_ptr<ModelBundleVS> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBufferIndices = std::move(modelBufferIndices);
	m_modelBundle        = std::move(bundle);
}

void ModelBundleVSIndividual::Draw(
	const D3DCommandList& graphicsList, UINT constantsRootIndex
) const noexcept {
	ID3D12GraphicsCommandList* cmdList = graphicsList.Get();
	const auto& models                 = m_modelBundle->GetModels();

	for (size_t index = 0; index < std::size(models); ++index)
	{
		constexpr UINT pushConstantCount = GetConstantCount();

		cmdList->SetGraphicsRoot32BitConstants(
			constantsRootIndex, pushConstantCount, &m_modelBufferIndices[index], 0u
		);

		const D3D12_DRAW_INDEXED_ARGUMENTS meshArgs = GetDrawIndexedIndirectCommand(models[index]);

		cmdList->DrawIndexedInstanced(
			meshArgs.IndexCountPerInstance, meshArgs.InstanceCount, meshArgs.StartIndexLocation,
			meshArgs.BaseVertexLocation, meshArgs.StartInstanceLocation
		);
	}
}

// Model Bundle MS
void ModelBundleMSIndividual::SetModelBundle(
	std::shared_ptr<ModelBundleMS> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBundle        = std::move(bundle);
	m_modelBufferIndices = std::move(modelBufferIndices);
}

void ModelBundleMSIndividual::Draw(
	const D3DCommandList& graphicsList, UINT constantsASRootIndex, UINT constantsMSRootIndex
) const noexcept {
	ID3D12GraphicsCommandList6* cmdList = graphicsList.Get();
	const auto& models                  = m_modelBundle->GetModels();

	for (size_t index = 0u; index < std::size(models); ++index)
	{
		const auto& model = models[index];

		constexpr UINT pushConstantMSCount = GetMSConstantCount();
		constexpr UINT pushConstantASCount = GetASConstantCount();

		const MeshDetailsMS meshDetails    = model->GetMeshDetailsMS();

		const ModelDetailsMS msConstants
		{
			.meshDetails      = meshDetails,
			.modelBufferIndex = m_modelBufferIndices[index]
		};

		const ModelDetailsAS asConstants
		{
			.meshletCount = meshDetails.meshletCount
		};

		cmdList->SetGraphicsRoot32BitConstants(
			constantsASRootIndex, pushConstantASCount, &asConstants, 0u
		);
		cmdList->SetGraphicsRoot32BitConstants(
			constantsMSRootIndex, pushConstantMSCount, &msConstants, 0u
		);

		// If we have an Amplification shader, this will launch an amplification global workGroup.
		// We would want each Amplification shader lane to process a meshlet and launch the necessary
		// Mesh Shader workGroups. On Nvdia we can have a maximum of 32 lanes active
		// in a wave and 64 on AMD. So, a workGroup will be able to work on 32/64
		// meshlets concurrently.
		const UINT amplficationGroupCount = DivRoundUp(
			meshDetails.meshletCount, s_amplificationLaneCount
		);

		cmdList->DispatchMesh(amplficationGroupCount, 1u, 1u);
		// It might be worth checking if we are reaching the Group Count Limit and if needed
		// launch more Groups. Could achieve that by passing a GroupLaunch index.
	}
}

// Model Bundle CS Indirect
ModelBundleCSIndirect::ModelBundleCSIndirect()
	: m_modelBundleIndexSharedData{ nullptr, 0u, 0u }, m_cullingSharedData{ nullptr, 0u, 0u },
	m_argumentInputSharedData{}, m_modelBundle{}, m_modelIndices{},
	m_cullingData{
		std::make_unique<CullingData>(
			CullingData{
				.commandCount = 0u,
				.commandOffset = 0u
			}
		)
	}
{}

void ModelBundleCSIndirect::SetModelBundle(std::shared_ptr<ModelBundleVS> bundle) noexcept
{
	m_modelBundle = std::move(bundle);
}

void ModelBundleCSIndirect::CreateBuffers(
	StagingBufferManager& stagingBufferMan,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffer,
	SharedBufferGPU& cullingSharedBuffer, SharedBufferGPU& modelBundleIndexSharedBuffer,
	std::vector<std::uint32_t> modelIndices, TemporaryDataBufferGPU& tempBuffer
) {
	constexpr size_t strideSize = sizeof(IndirectArgument);
	const auto argumentCount    = static_cast<std::uint32_t>(std::size(m_modelBundle->GetModels()));
	m_cullingData->commandCount = argumentCount;

	const auto argumentBufferSize         = static_cast<UINT64>(strideSize * argumentCount);
	const auto cullingDataSize            = static_cast<UINT64>(sizeof(CullingData));
	const auto modelBundleIndicesDataSize = static_cast<UINT64>(sizeof(std::uint32_t) * argumentCount);

	{
		const size_t argumentInputBufferCount = std::size(argumentInputSharedBuffer);
		m_argumentInputSharedData.resize(argumentInputBufferCount);

		for (size_t index = 0u; index < argumentInputBufferCount; ++index)
		{
			SharedBufferData& argumentInputSharedData = m_argumentInputSharedData[index];

			argumentInputSharedData = argumentInputSharedBuffer[index].AllocateAndGetSharedData(
				argumentBufferSize
			);

			// The offset on each sharedBuffer should be the same.
			m_cullingData->commandOffset
				= static_cast<std::uint32_t>(argumentInputSharedData.offset / strideSize);
		}
	}

	m_cullingSharedData          = cullingSharedBuffer.AllocateAndGetSharedData(
		cullingDataSize, tempBuffer
	);
	m_modelBundleIndexSharedData = modelBundleIndexSharedBuffer.AllocateAndGetSharedData(
		modelBundleIndicesDataSize, tempBuffer
	);

	const auto modelBundleIndex = GetModelBundleIndex();

	// Each thread will process a single model independently. And since we are trying to
	// cull all of the models across all of the bundles with a single call to dispatch, we can't
	// set the index as constantData per bundle. So, we will be giving each model the index
	// of its bundle so each thread can work independently.
	auto modelBundleIndicesInModels = std::vector<std::uint32_t>(argumentCount, modelBundleIndex);
	std::shared_ptr<std::uint8_t[]> modelBundleIndicesInModelsData = CopyVectorToSharedPtr(
		modelBundleIndicesInModels
	);

	stagingBufferMan.AddBuffer(
		std::move(m_cullingData), cullingDataSize, m_cullingSharedData.bufferData,
		m_cullingSharedData.offset, tempBuffer
	);
	stagingBufferMan.AddBuffer(
		std::move(modelBundleIndicesInModelsData), modelBundleIndicesDataSize,
		m_modelBundleIndexSharedData.bufferData, m_modelBundleIndexSharedData.offset,
		tempBuffer
	);

	m_modelIndices = std::move(modelIndices);
}

void ModelBundleCSIndirect::Update(size_t bufferIndex) const noexcept
{
	const SharedBufferData& argumentInputSharedData = m_argumentInputSharedData[bufferIndex];

	std::uint8_t* argumentInputStart
		= argumentInputSharedData.bufferData->CPUHandle() + argumentInputSharedData.offset;

	constexpr size_t argumentStride = sizeof(IndirectArgument);
	size_t modelOffset              = 0u;
	const auto& models              = m_modelBundle->GetModels();

	const size_t modelCount         = std::size(models);

	for (size_t index = 0u; index < modelCount; ++index)
	{
		const std::shared_ptr<ModelVS>& model = models[index];
		const UINT modelIndex                 = m_modelIndices[index];

		IndirectArgument arguments
		{
			.modelIndex    = modelIndex,
			.drawArguments = ModelBundle::GetDrawIndexedIndirectCommand(model)
		};

		memcpy(argumentInputStart + modelOffset, &arguments, argumentStride);

		modelOffset += argumentStride;
	}
}

// Model Bundle VS Indirect
ModelBundleVSIndirect::ModelBundleVSIndirect()
	: ModelBundle{}, m_modelCount{ 0u }, m_modelBundle{}, m_argumentOutputSharedData{},
	m_counterSharedData{}, m_bundleID{ 0u }
{}

void ModelBundleVSIndirect::SetModelBundle(std::shared_ptr<ModelBundleVS> bundle) noexcept
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

// Model Buffers
void ModelBuffers::CreateBuffer(size_t modelCount)
{
	// Vertex Data
	{
		constexpr size_t strideSize = GetVertexStride();

		m_modelBuffersInstanceSize        = static_cast<UINT64>(strideSize * modelCount);
		const UINT64 modelBufferTotalSize = m_modelBuffersInstanceSize * m_bufferInstanceCount;

		m_buffers.Create(modelBufferTotalSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	// Fragment Data
	{
		constexpr size_t strideSize = GetPixelStride();

		m_modelBuffersPixelInstanceSize   = static_cast<UINT64>(strideSize * modelCount);
		const UINT64 modelBufferTotalSize = m_modelBuffersPixelInstanceSize * m_bufferInstanceCount;

		m_pixelModelBuffers.Create(modelBufferTotalSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	}
}

void ModelBuffers::SetDescriptor(
	D3DDescriptorManager& descriptorManager, UINT64 frameIndex, size_t registerSlot,
	size_t registerSpace, bool graphicsQueue
) const {
	const auto bufferOffset = static_cast<UINT64>(frameIndex * m_modelBuffersInstanceSize);

	descriptorManager.SetRootSRV(
		registerSlot, registerSpace, m_buffers.GetGPUAddress() + bufferOffset, graphicsQueue
	);
}

void ModelBuffers::SetPixelDescriptor(
	D3DDescriptorManager& descriptorManager, UINT64 frameIndex, size_t registerSlot,
	size_t registerSpace
) const {
	const auto bufferOffset = static_cast<UINT64>(frameIndex * m_modelBuffersPixelInstanceSize);

	descriptorManager.SetRootSRV(
		registerSlot, registerSpace, m_pixelModelBuffers.GetGPUAddress() + bufferOffset, true
	);
}

void ModelBuffers::Update(UINT64 bufferIndex) const noexcept
{
	// Vertex Data
	std::uint8_t* vertexBufferOffset  = m_buffers.CPUHandle() + bufferIndex * m_modelBuffersInstanceSize;
	constexpr size_t vertexStrideSize = GetVertexStride();
	size_t vertexModelOffset          = 0u;

	// Pixel Data
	std::uint8_t* pixelBufferOffset
		= m_pixelModelBuffers.CPUHandle() + bufferIndex * m_modelBuffersPixelInstanceSize;
	constexpr size_t pixelStrideSize = GetPixelStride();
	size_t pixelModelOffset          = 0u;

	const size_t modelCount = m_elements.GetCount();

	// All of the models will be here. Even after multiple models have been removed, there
	// should be null models there. It is necessary to keep them to preserve the model indices,
	// which is used to keep track of the models both on the CPU and the GPU side.
	for (size_t index = 0u; index < modelCount; ++index)
	{
		// Don't update the data if the model is not in use. Could use this functionality to
		// temporarily hide models later.
		if (m_elements.IsInUse(index))
		{
			const auto& model = m_elements.at(index);

			// Vertex Data
			{
				const ModelVertexData modelVertexData{
					.modelMatrix   = model->GetModelMatrix(),
					.modelOffset   = model->GetModelOffset(),
					.materialIndex = model->GetMaterialIndex()
				};

				memcpy(vertexBufferOffset + vertexModelOffset, &modelVertexData, vertexStrideSize);
			}

			// Pixel Data
			{
				const ModelPixelData modelPixelData{
					.diffuseTexUVInfo  = model->GetDiffuseUVInfo(),
					.specularTexUVInfo = model->GetSpecularUVInfo(),
					.diffuseTexIndex   = model->GetDiffuseIndex(),
					.specularTexIndex  = model->GetSpecularIndex()
				};

				memcpy(pixelBufferOffset + pixelModelOffset, &modelPixelData, pixelStrideSize);
			}
		}
		// The offsets need to be always increased to keep them consistent.
		vertexModelOffset += vertexStrideSize;
		pixelModelOffset  += pixelStrideSize;
	}
}

// Model Manager VS Individual
ModelManagerVSIndividual::ModelManagerVSIndividual(
	ID3D12Device5* device, MemoryManager* memoryManager, std::uint32_t frameCount
) : ModelManager{ device, memoryManager, frameCount }, m_constantsRootIndex{ 0u },
	m_vertexBuffer{ device, memoryManager }, m_indexBuffer{ device, memoryManager }
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
	std::shared_ptr<ModelBundleVS>&& modelBundle, TemporaryDataBufferGPU&// Not needed in this system.
) const noexcept {
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));
}

void ModelManagerVSIndividual::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles[bundleIndex];

	const auto& modelIndices = modelBundle.GetIndices();

	for (const auto& modelIndex : modelIndices)
		m_modelBuffers.Remove(modelIndex);
}

void ModelManagerVSIndividual::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	auto& meshManager = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData = meshManager.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& indexSharedData = meshManager.GetIndexSharedData();
		m_indexBuffer.RelinquishMemory(indexSharedData);
	}
}

void ModelManagerVSIndividual::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, tempBuffer
	);
}

void ModelManagerVSIndividual::CopyTempData(const D3DCommandList& copyList) noexcept
{
	if (m_tempCopyNecessary)
	{
		m_indexBuffer.CopyOldBuffer(copyList);
		m_vertexBuffer.CopyOldBuffer(copyList);

		m_tempCopyNecessary = false;
	}
}

void ModelManagerVSIndividual::SetDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers,
	size_t vsRegisterSpace, size_t psRegisterSpace
) {
	const auto frameCount            = std::size(descriptorManagers);
	constexpr UINT pushConstantCount = ModelBundleVSIndividual::GetConstantCount();

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];

		descriptorManager.AddConstants(
			s_constantDataCBVRegisterSlot, vsRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersGraphicsSRVRegisterSlot, vsRegisterSpace, D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersPixelSRVRegisterSlot, psRegisterSpace, D3D12_SHADER_VISIBILITY_PIXEL
		);
	}
}

void ModelManagerVSIndividual::SetDescriptors(
	std::vector<D3DDescriptorManager>& descriptorManagers,
	size_t vsRegisterSpace, size_t psRegisterSpace
) {
	const auto frameCount = std::size(descriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];
		const auto frameIndex                   = static_cast<UINT64>(index);

		m_modelBuffers.SetDescriptor(
			descriptorManager, frameIndex, s_modelBuffersGraphicsSRVRegisterSlot, vsRegisterSpace, true
		);
		m_modelBuffers.SetPixelDescriptor(
			descriptorManager, frameIndex, s_modelBuffersPixelSRVRegisterSlot, psRegisterSpace
		);
	}
}

void ModelManagerVSIndividual::Draw(const D3DCommandList& graphicsList) const noexcept
{
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	GraphicsPipelineVertexShader::SetIATopology(graphicsList);

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsList, previousPSOIndex);

		// Mesh
		BindMesh(modelBundle, graphicsList);

		// Model
		modelBundle.Draw(graphicsList, m_constantsRootIndex);
	}
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	ID3D12Device5* device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
	std::uint32_t frameCount
) : ModelManager{ device, memoryManager, frameCount },
	m_stagingBufferMan{ stagingBufferMan }, m_argumentInputBuffers{}, m_argumentOutputBuffers{},
	m_cullingDataBuffer{ device, memoryManager }, m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
	m_meshIndexBuffer{ device, memoryManager, frameCount }, m_meshDetailsBuffer{ device, memoryManager },
	m_vertexBuffer{ device, memoryManager },
	m_indexBuffer{ device, memoryManager },
	m_modelBundleIndexBuffer{ device, memoryManager },
	m_meshBoundsBuffer{ device, memoryManager },
	m_computeRootSignature{ nullptr }, m_computePipeline{},
	m_dispatchXCount{ 0u }, m_argumentCount{ 0u }, m_constantsVSRootIndex{ 0u },
	m_constantsCSRootIndex{ 0u }, m_modelBundlesCS{}
{
	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_argumentInputBuffers.emplace_back(
			SharedBufferCPU{ m_device, m_memoryManager, D3D12_RESOURCE_STATE_GENERIC_READ }
		);
		m_argumentOutputBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				m_device, m_memoryManager, D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			}
		);
		// Doing the resetting on the Compute queue, so CG should be fine.
		m_counterBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				m_device, m_memoryManager, D3D12_RESOURCE_STATE_COMMON,
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

void ModelManagerVSIndirect::ShaderPathSet()
{
	// Must create the pipeline object after the shader path has been set.
	m_computePipeline.Create(
		m_device, m_computeRootSignature, L"VertexShaderCSIndirect", m_shaderPath
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
	std::shared_ptr<ModelBundleVS>&& modelBundle, TemporaryDataBufferGPU& tempBuffer
) {
	ModelBundleCSIndirect modelBundleCS{};

	modelBundleCS.SetModelBundle(modelBundle);
	modelBundleObj.SetModelBundle(std::move(modelBundle));

	modelBundleCS.CreateBuffers(
		*m_stagingBufferMan, m_argumentInputBuffers, m_cullingDataBuffer, m_modelBundleIndexBuffer,
		std::move(modelIndices), tempBuffer
	);

	modelBundleObj.CreateBuffers(
		m_argumentOutputBuffers, m_counterBuffers, modelBundleCS.GetModelCount()
	);

	UpdateCounterResetValues();

	modelBundleObj.SetID(static_cast<std::uint32_t>(modelBundleCS.GetID()));

	const auto modelBundleIndexInBuffer = modelBundleCS.GetModelBundleIndex();

	m_meshIndexBuffer.Add(modelBundleIndexInBuffer);

	m_argumentCount += modelBundleCS.GetModelCount();

	m_modelBundlesCS.emplace_back(std::move(modelBundleCS));

	UpdateDispatchX();
}

void ModelManagerVSIndirect::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles.at(bundleIndex);

	{
		const std::vector<SharedBufferData>& argumentOutputSharedData
			= modelBundle.GetArgumentOutputSharedData();

		for (size_t index = 0u; index < std::size(m_argumentOutputBuffers); ++index)
			m_argumentOutputBuffers[index].RelinquishMemory(argumentOutputSharedData[index]);

		const std::vector<SharedBufferData>& counterSharedData = modelBundle.GetCounterSharedData();

		for (size_t index = 0u; index < std::size(m_counterBuffers); ++index)
			m_counterBuffers[index].RelinquishMemory(counterSharedData[index]);
	}

	const auto bundleID = static_cast<std::uint32_t>(modelBundle.GetID());

	std::erase_if(
		m_modelBundlesCS,
		[bundleID, this]
		(const ModelBundleCSIndirect& bundle)
		{
			const bool result = bundleID == bundle.GetID();

			if (result)
			{
				{
					const std::vector<SharedBufferData>& argumentInputSharedData
						= bundle.GetArgumentInputSharedData();

					for (size_t index = 0u; index < std::size(m_argumentInputBuffers); ++index)
						m_argumentInputBuffers[index].RelinquishMemory(argumentInputSharedData[index]);
				}

				m_cullingDataBuffer.RelinquishMemory(bundle.GetCullingSharedData());
				m_modelBundleIndexBuffer.RelinquishMemory(bundle.GetModelBundleIndexSharedData());

				const auto& modelIndices = bundle.GetModelIndices();

				for (const auto& modelIndex : modelIndices)
					m_modelBuffers.Remove(modelIndex);

				m_argumentCount -= static_cast<std::uint32_t>(std::size(modelIndices));

				UpdateDispatchX();
			}

			return result;
		}
	);
}

void ModelManagerVSIndirect::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	auto& meshManager = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData = meshManager.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& indexSharedData = meshManager.GetIndexSharedData();
		m_indexBuffer.RelinquishMemory(indexSharedData);

		const SharedBufferData& meshBoundsSharedData = meshManager.GetBoundsSharedData();
		m_meshBoundsBuffer.RelinquishMemory(meshBoundsSharedData);
	}
}

void ModelManagerVSIndirect::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, m_meshBoundsBuffer,
		tempBuffer
	);

	// This function is also used by the the Add function. Calling it early
	// here should make it so it won't be called again the in Add function.
	// But the returned value should be the same.
	const size_t meshIndex = m_meshBundles.GetNextFreeIndex();

	BoundsDetails details = meshManager.GetBoundsDetails();
	m_meshDetailsBuffer.Add(meshIndex, details);
}

void ModelManagerVSIndirect::_updatePerFrame(UINT64 frameIndex) const noexcept
{
	std::uint8_t* bufferOffsetPtr = m_meshIndexBuffer.GetInstancePtr(frameIndex);
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
		bundle.Update(static_cast<size_t>(frameIndex));

		const std::uint32_t meshIndex        = bundle.GetMeshIndex();
		const std::uint32_t modelBundleIndex = bundle.GetModelBundleIndex();

		bufferOffset = strideSize * modelBundleIndex;

		memcpy(bufferOffsetPtr + bufferOffset, &meshIndex, strideSize);
	}
}

void ModelManagerVSIndirect::SetDescriptorLayoutVS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace,
	size_t psRegisterSpace
) const noexcept {
	const auto frameCount            = std::size(descriptorManagers);
	constexpr auto pushConstantCount = ModelBundleVSIndirect::GetConstantCount();

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];

		descriptorManager.AddConstants(
			s_constantDataVSCBVRegisterSlot, vsRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersGraphicsSRVRegisterSlot, vsRegisterSpace, D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersPixelSRVRegisterSlot, psRegisterSpace, D3D12_SHADER_VISIBILITY_PIXEL
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorsVS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace,
	size_t psRegisterSpace
) const {
	const auto frameCount = std::size(descriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];
		const auto frameIndex                   = static_cast<UINT64>(index);

		m_modelBuffers.SetDescriptor(
			descriptorManager, frameIndex, s_modelBuffersGraphicsSRVRegisterSlot, vsRegisterSpace, true
		);
		m_modelBuffers.SetPixelDescriptor(
			descriptorManager, frameIndex, s_modelBuffersPixelSRVRegisterSlot, psRegisterSpace
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorLayoutCS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const noexcept {
	const auto frameCount            = std::size(descriptorManagers);
	constexpr auto pushConstantCount = GetConstantCount();

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];

		descriptorManager.AddConstants(
			s_constantDataCSCBVRegisterSlot, csRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersCSSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
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
			s_modelBundleIndexSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_meshBoundingSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_meshIndexSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_meshDetailsSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorsCSOfModels(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const {
	const auto frameCount = std::size(descriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];
		const auto frameIndex                   = static_cast<UINT64>(index);

		m_modelBuffers.SetDescriptor(
			descriptorManager, frameIndex, s_modelBuffersCSSRVRegisterSlot, csRegisterSpace, false
		);

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
			s_modelBundleIndexSRVRegisterSlot, csRegisterSpace,
			m_modelBundleIndexBuffer.GetGPUAddress(), false
		);

		m_meshIndexBuffer.SetRootSRVCom(descriptorManager, s_meshIndexSRVRegisterSlot, csRegisterSpace);
		m_meshDetailsBuffer.SetRootSRVCom(
			descriptorManager, s_meshDetailsSRVRegisterSlot, csRegisterSpace
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorsCSOfMeshes(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const {
	for (auto& descriptorManager : descriptorManagers)
		descriptorManager.SetRootSRV(
			s_meshBoundingSRVRegisterSlot, csRegisterSpace, m_meshBoundsBuffer.GetGPUAddress(), false
		);
}

void ModelManagerVSIndirect::Dispatch(const D3DCommandList& computeList) const noexcept
{
	ID3D12GraphicsCommandList* cmdList = computeList.Get();

	m_computePipeline.Bind(computeList);

	{
		constexpr auto pushConstantCount = GetConstantCount();

		constexpr Bounds maxBounds
		{
			.maxXBounds = XBOUNDS,
			.maxYBounds = YBOUNDS,
			.maxZBounds = ZBOUNDS
		};

		const ConstantData constantData
		{
			.maxBounds  = maxBounds,
			.modelCount = m_argumentCount
		};

		cmdList->SetComputeRoot32BitConstants(
			m_constantsCSRootIndex, pushConstantCount, &constantData, 0u
		);
	}

	cmdList->Dispatch(m_dispatchXCount, 1u, 1u);
}

void ModelManagerVSIndirect::Draw(
	size_t frameIndex, const D3DCommandList& graphicsList, ID3D12CommandSignature* commandSignature
) const noexcept {
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	GraphicsPipelineVertexShader::SetIATopology(graphicsList);

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsList, previousPSOIndex);

		// Mesh
		BindMesh(modelBundle, graphicsList);

		// Model
		modelBundle.Draw(frameIndex, commandSignature, graphicsList);
	}
}

void ModelManagerVSIndirect::CopyTempBuffers(const D3DCommandList& copyList) noexcept
{
	if (m_tempCopyNecessary)
	{
		m_cullingDataBuffer.CopyOldBuffer(copyList);
		m_vertexBuffer.CopyOldBuffer(copyList);
		m_indexBuffer.CopyOldBuffer(copyList);
		m_modelBundleIndexBuffer.CopyOldBuffer(copyList);
		m_meshBoundsBuffer.CopyOldBuffer(copyList);

		// I don't think copying is needed for the Output Argument
		// and the counter buffers. As their data will be only
		// needed on the same frame and not afterwards.

		m_tempCopyNecessary = false;
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
ModelManagerMS::ModelManagerMS(
	ID3D12Device5* device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
	std::uint32_t frameCount
) : ModelManager{ device, memoryManager, frameCount },
	m_constantsMSRootIndex{ 0u },
	m_constantsASRootIndex{ 0u },
	m_stagingBufferMan{ stagingBufferMan },
	m_meshletBuffer{ device, memoryManager },
	m_vertexBuffer{ device, memoryManager },
	m_vertexIndicesBuffer{ device, memoryManager },
	m_primIndicesBuffer{ device, memoryManager }
{}

void ModelManagerMS::ConfigureModelBundle(
	ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundleMS>&& modelBundle, [[maybe_unused]] TemporaryDataBufferGPU& tempBuffer
) {
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));
}

void ModelManagerMS::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles[bundleIndex];

	const auto& modelIndices = modelBundle.GetIndices();

	for (std::uint32_t modelIndex : modelIndices)
		m_modelBuffers.Remove(modelIndex);
}

void ModelManagerMS::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	auto& meshManager = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData = meshManager.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& vertexIndicesSharedData = meshManager.GetVertexIndicesSharedData();
		m_vertexIndicesBuffer.RelinquishMemory(vertexIndicesSharedData);

		const SharedBufferData& primIndicesSharedData = meshManager.GetPrimIndicesSharedData();
		m_primIndicesBuffer.RelinquishMemory(primIndicesSharedData);

		const SharedBufferData& meshletSharedData = meshManager.GetMeshletSharedData();
		m_meshletBuffer.RelinquishMemory(meshletSharedData);
	}
}

void ModelManagerMS::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerMeshShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_vertexIndicesBuffer,
		m_primIndicesBuffer, m_meshletBuffer, tempBuffer
	);
}

void ModelManagerMS::_setGraphicsConstantRootIndex(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) noexcept {
	m_constantsMSRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataMSCBVRegisterSlot, constantsRegisterSpace
	);
	m_constantsASRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataASCBVRegisterSlot, constantsRegisterSpace
	);
}

void ModelManagerMS::CopyTempBuffers(const D3DCommandList& copyList) noexcept
{
	if (m_tempCopyNecessary)
	{
		m_meshletBuffer.CopyOldBuffer(copyList);
		m_vertexBuffer.CopyOldBuffer(copyList);
		m_vertexIndicesBuffer.CopyOldBuffer(copyList);
		m_primIndicesBuffer.CopyOldBuffer(copyList);

		m_tempCopyNecessary = false;
	}
}

void ModelManagerMS::SetDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace,
	size_t psRegisterSpace
) const noexcept {
	const auto frameCount               = std::size(descriptorManagers);
	constexpr UINT meshConstantCount    = MeshManagerMeshShader::GetConstantCount();
	constexpr UINT modelASConstantCount = ModelBundleMSIndividual::GetASConstantCount();
	constexpr UINT modelMSConstantCount = ModelBundleMSIndividual::GetMSConstantCount();

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];

		descriptorManager.AddConstants(
			s_constantDataASCBVRegisterSlot, msRegisterSpace, modelASConstantCount,
			D3D12_SHADER_VISIBILITY_AMPLIFICATION
		);
		descriptorManager.AddConstants(
			s_constantDataMSCBVRegisterSlot, msRegisterSpace, meshConstantCount + modelMSConstantCount,
			D3D12_SHADER_VISIBILITY_MESH
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersGraphicsSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_MESH
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersPixelSRVRegisterSlot, psRegisterSpace, D3D12_SHADER_VISIBILITY_PIXEL
		);
		descriptorManager.AddRootSRV(
			s_meshletBufferSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_MESH
		);
		descriptorManager.AddRootSRV(
			s_vertexBufferSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_MESH
		);
		descriptorManager.AddRootSRV(
			s_vertexIndicesBufferSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_MESH
		);
		descriptorManager.AddRootSRV(
			s_primIndicesBufferSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_MESH
		);
	}
}

void ModelManagerMS::SetDescriptorsOfModels(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace,
	size_t psRegisterSpace
) const {
	const auto frameCount = std::size(descriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];
		const auto frameIndex                   = static_cast<UINT64>(index);

		m_modelBuffers.SetDescriptor(
			descriptorManager, frameIndex, s_modelBuffersGraphicsSRVRegisterSlot, msRegisterSpace, true
		);
		m_modelBuffers.SetPixelDescriptor(
			descriptorManager, frameIndex, s_modelBuffersPixelSRVRegisterSlot, psRegisterSpace
		);
	}
}

void ModelManagerMS::SetDescriptorsOfMeshes(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace
) const {
	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
	{
		descriptorManager.SetRootSRV(
			s_vertexBufferSRVRegisterSlot, msRegisterSpace, m_vertexBuffer.GetGPUAddress(), true
		);
		descriptorManager.SetRootSRV(
			s_vertexIndicesBufferSRVRegisterSlot, msRegisterSpace, m_vertexIndicesBuffer.GetGPUAddress(),
			true
		);
		descriptorManager.SetRootSRV(
			s_primIndicesBufferSRVRegisterSlot, msRegisterSpace, m_primIndicesBuffer.GetGPUAddress(),
			true
		);
		descriptorManager.SetRootSRV(
			s_meshletBufferSRVRegisterSlot, msRegisterSpace, m_meshletBuffer.GetGPUAddress(), true
		);
	}
}

void ModelManagerMS::Draw(const D3DCommandList& graphicsList) const noexcept
{
	auto previousPSOIndex              = std::numeric_limits<size_t>::max();
	ID3D12GraphicsCommandList* cmdList = graphicsList.Get();

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsList, previousPSOIndex);

		{
			const size_t meshIndex                  = modelBundle.GetMeshIndex();
			const MeshManagerMeshShader& meshBundle = m_meshBundles.at(meshIndex);

			constexpr UINT constBufferOffset        = ModelBundleMSIndividual::GetMSConstantCount();
			constexpr UINT constBufferCount         = MeshManagerMeshShader::GetConstantCount();

			const MeshManagerMeshShader::MeshDetails meshDetails = meshBundle.GetMeshDetails();

			cmdList->SetGraphicsRoot32BitConstants(
				m_constantsMSRootIndex, constBufferCount, &meshDetails, constBufferOffset
			);
		}

		// Model
		modelBundle.Draw(graphicsList, m_constantsASRootIndex, m_constantsMSRootIndex);
	}
}
