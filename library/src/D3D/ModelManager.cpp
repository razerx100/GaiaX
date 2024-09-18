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
	rootSignatureDynamic.CompileSignature(
		RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::UnboundArray
	);

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

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	ID3D12Device5* device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
	std::uint32_t frameCount
) : ModelManager{ device, memoryManager, frameCount },
	m_stagingBufferMan{ stagingBufferMan }, m_argumentInputBuffers{}, m_argumentOutputBuffers{},
	m_modelIndicesVSBuffers{},
	m_cullingDataBuffer{ device, memoryManager }, m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
	m_meshIndexBuffer{ device, memoryManager, frameCount }, m_meshDetailsBuffer{ device, memoryManager },
	m_modelIndicesCSBuffer{ device, memoryManager },
	m_vertexBuffer{ device, memoryManager },
	m_indexBuffer{ device, memoryManager },
	m_modelBundleIndexBuffer{ device, memoryManager },
	m_meshBoundsBuffer{ device, memoryManager },
	m_computeRootSignature{}, m_computePipeline{}, m_commandSignature{},
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
		m_modelIndicesVSBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				m_device, m_memoryManager, D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_FLAG_NONE
			}
		);
	}
}

void ModelManagerVSIndirect::CreateRootSignatureImpl(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) {
	D3DRootSignatureDynamic rootSignatureDynamic{};

	rootSignatureDynamic.PopulateFromLayouts(descriptorManager.GetLayouts());
	rootSignatureDynamic.CompileSignature(
		RSCompileFlagBuilder{}.VertexShader(), BindlessLevel::UnboundArray
	);

	m_graphicsRootSignature.CreateSignature(m_device, rootSignatureDynamic);

	m_constantsVSRootIndex = descriptorManager.GetRootIndex(
		s_constantDataVSRegisterSlot, constantsRegisterSpace
	);

	// Command Signature
	D3D12_INDIRECT_ARGUMENT_DESC argumentDesc{ .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED };

	D3D12_COMMAND_SIGNATURE_DESC signatureDesc
	{
		.ByteStride       = static_cast<UINT>(sizeof(D3D12_DRAW_INDEXED_ARGUMENTS)),
		.NumArgumentDescs = 1u,
		.pArgumentDescs   = &argumentDesc
	};

	// If the argumentDesc contains only has the draw type, the rootSignature should be null.
	m_device->CreateCommandSignature(&signatureDesc, nullptr, IID_PPV_ARGS(&m_commandSignature));
}

void ModelManagerVSIndirect::CreatePipelineCS(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) {
	D3DRootSignatureDynamic rootSignatureDynamic{};

	rootSignatureDynamic.PopulateFromLayouts(descriptorManager.GetLayouts());
	rootSignatureDynamic.CompileSignature(
		RSCompileFlagBuilder{}.ComputeShader(), BindlessLevel::UnboundArray
	);

	m_graphicsRootSignature.CreateSignature(m_device, rootSignatureDynamic);

	m_constantsVSRootIndex = descriptorManager.GetRootIndex(
		s_constantDataCSRegisterSlot, constantsRegisterSpace
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
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));

	modelBundleCS.SetID(static_cast<std::uint32_t>(modelBundleObj.GetID()));

	modelBundleObj.CreateBuffers(m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesVSBuffers);

	UpdateCounterResetValues();

	modelBundleCS.CreateBuffers(
		*m_stagingBufferMan, m_argumentInputBuffers, m_cullingDataBuffer, m_modelBundleIndexBuffer,
		m_modelIndicesCSBuffer, modelBundleObj.GetModelIndices(), tempBuffer
	);

	const auto modelBundleIndexInBuffer = modelBundleCS.GetModelBundleIndex();

	m_meshIndexBuffer.Add(modelBundleIndexInBuffer);

	m_modelBundlesCS.emplace_back(std::move(modelBundleCS));

	m_argumentCount += modelBundleObj.GetModelCount();

	UpdateDispatchX();
}

void ModelManagerVSIndirect::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles.at(bundleIndex);
	const auto& modelIndices = modelBundle.GetModelIndices();

	for (const auto& modelIndex : modelIndices)
		m_modelBuffers.Remove(modelIndex);

	m_argumentCount -= static_cast<std::uint32_t>(std::size(modelIndices));

	UpdateDispatchX();

	const auto bundleID = static_cast<std::uint32_t>(modelBundle.GetID());

	{
		const std::vector<SharedBufferData>& argumentOutputSharedData
			= modelBundle.GetArgumentOutputSharedData();

		for (size_t index = 0u; index < std::size(m_argumentOutputBuffers); ++index)
			m_argumentOutputBuffers[index].RelinquishMemory(argumentOutputSharedData[index]);

		const std::vector<SharedBufferData>& counterSharedData = modelBundle.GetCounterSharedData();

		for (size_t index = 0u; index < std::size(m_counterBuffers); ++index)
			m_counterBuffers[index].RelinquishMemory(counterSharedData[index]);

		const std::vector<SharedBufferData>& modelIndicesSharedData
			= modelBundle.GetModelIndicesSharedData();

		for (size_t index = 0u; index < std::size(m_modelIndicesVSBuffers); ++index)
			m_modelIndicesVSBuffers[index].RelinquishMemory(modelIndicesSharedData[index]);
	}

	std::erase_if(
		m_modelBundlesCS,
		[bundleID, &argumentInputs = m_argumentInputBuffers,
		&cullingData = m_cullingDataBuffer,
		&bundleIndices = m_modelBundleIndexBuffer, &modelIndicesBuffer = m_modelIndicesCSBuffer]
		(const ModelBundleCSIndirect& bundle)
		{
			const bool result = bundleID == bundle.GetID();

			if (result)
			{
				{
					const std::vector<SharedBufferData>& argumentInputSharedData
						= bundle.GetArgumentInputSharedData();

					for (size_t index = 0u; index < std::size(argumentInputs); ++index)
						argumentInputs[index].RelinquishMemory(argumentInputSharedData[index]);
				}

				cullingData.RelinquishMemory(bundle.GetCullingSharedData());
				bundleIndices.RelinquishMemory(bundle.GetModelBundleIndexSharedData());
				modelIndicesBuffer.RelinquishMemory(bundle.GetModelIndicesSharedData());
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
			s_constantDataVSRegisterSlot, vsRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersGraphicsRegisterSlot, vsRegisterSpace, D3D12_SHADER_VISIBILITY_VERTEX
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersPixelRegisterSlot, psRegisterSpace, D3D12_SHADER_VISIBILITY_PIXEL
		);
		descriptorManager.AddRootSRV(
			s_modelIndicesVSRegisterSlot, vsRegisterSpace, D3D12_SHADER_VISIBILITY_VERTEX
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorVS(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t vsRegisterSpace,
	size_t psRegisterSpace
) const {
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
		descriptorManager.SetRootSRV(
			s_modelIndicesVSRegisterSlot, vsRegisterSpace,
			m_modelIndicesVSBuffers[index].GetGPUAddress(), true
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
			s_constantDataCSRegisterSlot, csRegisterSpace, pushConstantCount,
			D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_modelBuffersComputeRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_argumentInputBufferRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_cullingDataBufferRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootUAV(
			s_argumenOutputRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootUAV(
			s_counterRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_modelIndicesCSRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_modelBundleIndexRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_meshBoundingRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_meshIndexRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_meshDetailsRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_modelIndicesVSCSRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorCSOfModels(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const {
	const auto frameCount = std::size(descriptorManagers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];
		const auto frameIndex                   = static_cast<UINT64>(index);

		m_modelBuffers.SetDescriptor(
			descriptorManager, frameIndex, s_modelBuffersComputeRegisterSlot, csRegisterSpace, false
		);

		descriptorManager.SetRootSRV(
			s_argumentInputBufferRegisterSlot, csRegisterSpace,
			m_argumentInputBuffers[index].GetGPUAddress(), false
		);
		descriptorManager.SetRootSRV(
			s_cullingDataBufferRegisterSlot, csRegisterSpace,
			m_cullingDataBuffer.GetGPUAddress(), false
		);
		descriptorManager.SetRootUAV(
			s_argumenOutputRegisterSlot, csRegisterSpace,
			m_argumentOutputBuffers[index].GetGPUAddress(), false
		);
		descriptorManager.SetRootUAV(
			s_counterRegisterSlot, csRegisterSpace, m_counterBuffers[index].GetGPUAddress(), false
		);
		descriptorManager.SetRootSRV(
			s_modelIndicesCSRegisterSlot, csRegisterSpace,
			m_modelIndicesCSBuffer.GetGPUAddress(), false
		);
		descriptorManager.SetRootSRV(
			s_modelBundleIndexRegisterSlot, csRegisterSpace,
			m_modelBundleIndexBuffer.GetGPUAddress(), false
		);
		descriptorManager.SetRootSRV(
			s_modelIndicesVSCSRegisterSlot, csRegisterSpace,
			m_modelIndicesVSBuffers[index].GetGPUAddress(), false
		);

		m_meshIndexBuffer.SetRootSRVCom(descriptorManager, s_meshIndexRegisterSlot, csRegisterSpace);
		m_meshDetailsBuffer.SetRootSRVCom(descriptorManager, s_meshDetailsRegisterSlot, csRegisterSpace);
	}
}

void ModelManagerVSIndirect::SetDescriptorCSOfMeshes(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const {
	for (auto& descriptorManager : descriptorManagers)
		descriptorManager.SetRootSRV(
			s_meshBoundingRegisterSlot, csRegisterSpace, m_meshBoundsBuffer.GetGPUAddress(), false
		);
}

void ModelManagerVSIndirect::Dispatch(const D3DCommandList& computeList) const noexcept
{
	ID3D12GraphicsCommandList* cmdList = computeList.Get();

	m_computePipeline.Bind(computeList);

	{
		constexpr auto pushConstantCount = GetConstantCount();

		constexpr ConstantData constantData
		{
			.maxXBounds = XBOUNDS,
			.maxYBounds = YBOUNDS,
			.maxZBounds = ZBOUNDS
		};

		cmdList->SetComputeRoot32BitConstants(
			m_constantsCSRootIndex, pushConstantCount, &constantData, 0u
		);
	}

	cmdList->Dispatch(m_dispatchXCount, 1u, 1u);
}

void ModelManagerVSIndirect::Draw(size_t frameIndex, const D3DCommandList& graphicsList) const noexcept
{
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsList, previousPSOIndex);

		// Mesh
		BindMesh(modelBundle, graphicsList);

		// Model
		modelBundle.Draw(frameIndex, m_commandSignature.Get(), graphicsList, m_constantsVSRootIndex);
	}
}

void ModelManagerVSIndirect::CopyTempBuffers(const D3DCommandList& copyList) noexcept
{
	if (m_tempCopyNecessary)
	{
		m_cullingDataBuffer.CopyOldBuffer(copyList);
		m_modelIndicesCSBuffer.CopyOldBuffer(copyList);
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

GraphicsPipelineIndirectDraw ModelManagerVSIndirect::CreatePipelineObject()
{
	return GraphicsPipelineIndirectDraw{};
}
