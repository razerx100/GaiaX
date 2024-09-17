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
	const D3DCommandList& graphicsList, UINT constantsRootIndex
) const noexcept {
	ID3D12GraphicsCommandList6* cmdList = graphicsList.Get();
	const auto& models                  = m_modelBundle->GetModels();

	for (size_t index = 0u; index < std::size(models); ++index)
	{
		const auto& model = models[index];

		constexpr UINT pushConstantCount = GetConstantCount();
		const MeshDetailsMS meshDetails  = model->GetMeshDetailsMS();

		const ModelDetails msConstants
		{
			.modelBufferIndex = m_modelBufferIndices[index],
			.meshletOffset    = meshDetails.meshletOffset
		};

		constexpr UINT offset = MeshManagerMeshShader::GetConstantCount();

		cmdList->SetGraphicsRoot32BitConstants(
			constantsRootIndex, pushConstantCount, &msConstants, offset
		);

		// Unlike the Compute Shader where we process the data of a model with a thread, here
		// each group handles a Meshlet and its threads handle the vertices and primitives.
		// So, we need a thread group for each Meshlet.
		cmdList->DispatchMesh(meshDetails.meshletCount, 1u, 1u);
		// It might be worth checking if we are reaching the Group Count Limit and if needed
		// launch more Groups. Could achieve that by passing a GroupLaunch index.
	}
}

// Model Bundle CS Indirect
ModelBundleCSIndirect::ModelBundleCSIndirect()
	: m_modelBundleIndexSharedData{ nullptr, 0u, 0u },
	m_cullingSharedData{ nullptr, 0u, 0u }, m_modelIndicesSharedData{ nullptr, 0u, 0u },
	m_argumentInputSharedData{}, m_modelBundle{},
	m_cullingData{
		std::make_unique<CullingData>(
			CullingData{
				.commandCount = 0u,
				.commandOffset = 0u
			}
		)
	}, m_bundleID{ std::numeric_limits<std::uint32_t>::max() }
{}

void ModelBundleCSIndirect::SetModelBundle(std::shared_ptr<ModelBundleVS> bundle) noexcept
{
	m_modelBundle = std::move(bundle);
}

void ModelBundleCSIndirect::CreateBuffers(
	StagingBufferManager& stagingBufferMan,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffer,
	SharedBufferGPU& cullingSharedBuffer, SharedBufferGPU& modelBundleIndexSharedBuffer,
	SharedBufferGPU& modelIndicesBuffer, const std::vector<std::uint32_t>& modelIndices,
	TemporaryDataBufferGPU& tempBuffer
) {
	constexpr size_t strideSize = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
	const auto argumentCount    = static_cast<std::uint32_t>(std::size(m_modelBundle->GetModels()));
	m_cullingData->commandCount = argumentCount;

	const auto argumentBufferSize   = static_cast<UINT64>(strideSize * argumentCount);
	const auto cullingDataSize      = static_cast<UINT64>(sizeof(CullingData));
	const auto modelIndicesDataSize = static_cast<UINT64>(sizeof(std::uint32_t) * argumentCount);

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

	m_cullingSharedData = cullingSharedBuffer.AllocateAndGetSharedData(cullingDataSize, tempBuffer);
	m_modelBundleIndexSharedData = modelBundleIndexSharedBuffer.AllocateAndGetSharedData(
		modelIndicesDataSize, tempBuffer
	);
	m_modelIndicesSharedData = modelIndicesBuffer.AllocateAndGetSharedData(
		modelIndicesDataSize, tempBuffer
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

	std::shared_ptr<std::uint8_t[]> modelIndicesTempBuffer = CopyVectorToSharedPtr(modelIndices);

	stagingBufferMan.AddBuffer(
		std::move(modelIndicesTempBuffer), modelIndicesDataSize,
		m_modelIndicesSharedData.bufferData, m_modelIndicesSharedData.offset,
		tempBuffer
	);
	stagingBufferMan.AddBuffer(
		std::move(m_cullingData), cullingDataSize, m_cullingSharedData.bufferData,
		m_cullingSharedData.offset, tempBuffer
	);
	stagingBufferMan.AddBuffer(
		std::move(modelBundleIndicesInModelsData), modelIndicesDataSize,
		m_modelBundleIndexSharedData.bufferData, m_modelBundleIndexSharedData.offset,
		tempBuffer
	);
}

void ModelBundleCSIndirect::Update(size_t bufferIndex) const noexcept
{
	const SharedBufferData& argumentInputSharedData = m_argumentInputSharedData[bufferIndex];

	std::uint8_t* argumentInputStart
		= argumentInputSharedData.bufferData->CPUHandle() + argumentInputSharedData.offset;

	constexpr size_t argumentStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
	size_t modelOffset              = 0u;
	const auto& models              = m_modelBundle->GetModels();

	for (const auto& model : models)
	{
		const D3D12_DRAW_INDEXED_ARGUMENTS meshArgs = ModelBundle::GetDrawIndexedIndirectCommand(model);

		memcpy(argumentInputStart + modelOffset, &meshArgs, argumentStride);

		modelOffset += argumentStride;
	}
}

// Model Bundle VS Indirect
ModelBundleVSIndirect::ModelBundleVSIndirect()
	: ModelBundle{}, m_modelOffset{ 0u }, m_modelBundle{}, m_argumentOutputSharedData{},
	m_counterSharedData{}, m_modelIndicesSharedData{}, m_modelIndices{}
{}

void ModelBundleVSIndirect::SetModelBundle(
	std::shared_ptr<ModelBundleVS> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBundle = std::move(bundle);
	m_modelIndices = std::move(modelBufferIndices);
}

void ModelBundleVSIndirect::CreateBuffers(
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) {
	constexpr size_t argStrideSize      = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
	constexpr size_t indexStrideSize    = sizeof(std::uint32_t);
	const std::uint32_t modelCount      = GetModelCount();
	const auto argumentOutputBufferSize = static_cast<UINT64>(modelCount * argStrideSize);
	const auto modelIndiceBufferSize    = static_cast<UINT64>(modelCount * indexStrideSize);

	{
		const size_t argumentOutputBufferCount = std::size(argumentOutputSharedBuffers);
		m_argumentOutputSharedData.resize(argumentOutputBufferCount);

		for (size_t index = 0u; index < argumentOutputBufferCount; ++index)
		{
			SharedBufferData& argumentOutputSharedData = m_argumentOutputSharedData[index];

			argumentOutputSharedData = argumentOutputSharedBuffers[index].AllocateAndGetSharedData(
				argumentOutputBufferSize
			);

			// The offset on each sharedBuffer should be the same. But still need to keep track of each
			// of them because we will need the Buffer object to draw.
			m_modelOffset = static_cast<std::uint32_t>(argumentOutputSharedData.offset / argStrideSize);
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

	{
		const size_t modelIndicesBufferCount = std::size(modelIndicesSharedBuffers);
		m_modelIndicesSharedData.resize(modelIndicesBufferCount);

		for (size_t index = 0u; index < modelIndicesBufferCount; ++index)
			m_modelIndicesSharedData[index] = modelIndicesSharedBuffers[index].AllocateAndGetSharedData(
				modelIndiceBufferSize
			);
	}
}

void ModelBundleVSIndirect::Draw(
	size_t frameIndex, ID3D12CommandSignature* commandSignature,
	const D3DCommandList& graphicsList, UINT constantsRootIndex
) const noexcept {
	ID3D12GraphicsCommandList6* cmdList = graphicsList.Get();

	{
		constexpr UINT pushConstantCount = GetConstantCount();

		cmdList->SetGraphicsRoot32BitConstants(
			constantsRootIndex, pushConstantCount, &m_modelOffset, 0u
		);
	}

	const SharedBufferData& argumentOutputSharedData = m_argumentOutputSharedData[frameIndex];
	const SharedBufferData& counterSharedData        = m_counterSharedData[frameIndex];

	cmdList->ExecuteIndirect(
		commandSignature, GetModelCount(),
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

void ModelManagerVSIndividual::CreateRootSignatureImpl(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) {
	D3DRootSignatureDynamic rootSignatureDynamic{};

	rootSignatureDynamic.PopulateFromLayouts(descriptorManager.GetLayouts());
	m_graphicsRootSignature.CreateSignature(m_device, rootSignatureDynamic);

	m_constantsRootIndex = descriptorManager.GetRootIndex(
		s_constantDataRegisterSlot, constantsRegisterSpace
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
			s_constantDataRegisterSlot, vsRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersGraphicsRegisterSlot, vsRegisterSpace, D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersPixelRegisterSlot, psRegisterSpace, D3D12_SHADER_VISIBILITY_PIXEL
		);
	}
}

void ModelManagerVSIndividual::SetDescriptor(
	std::vector<D3DDescriptorManager>& descriptorManagers,
	size_t vsRegisterSpace, size_t psRegisterSpace
) {
	const auto frameCount = std::size(descriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];
		const auto frameIndex                   = static_cast<UINT64>(index);

		m_modelBuffers.SetDescriptor(
			descriptorManager, frameIndex, s_modelBuffersGraphicsRegisterSlot, vsRegisterSpace, true
		);
		m_modelBuffers.SetPixelDescriptor(
			descriptorManager, frameIndex, s_modelBuffersPixelRegisterSlot, psRegisterSpace
		);
	}
}

void ModelManagerVSIndividual::Draw(const D3DCommandList& graphicsList) const noexcept
{
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

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

GraphicsPipelineIndividualDraw ModelManagerVSIndividual::CreatePipelineObject()
{
	return GraphicsPipelineIndividualDraw{};
}

ModelBuffers ModelManagerVSIndividual::ConstructModelBuffers(
	ID3D12Device5* device, MemoryManager* memoryManager, std::uint32_t frameCount
) {
	return ModelBuffers{ device, memoryManager, frameCount };
}
