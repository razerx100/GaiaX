#include <ModelManager.hpp>
#include <D3DRootSignatureDynamic.hpp>

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

	const size_t modelCount          = m_elements.GetCount();

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
				const ModelVertexData modelVertexData
				{
					.modelMatrix   = model->GetModelMatrix(),
					.modelOffset   = model->GetModelOffset(),
					.materialIndex = model->GetMaterialIndex(),
					.meshIndex     = model->GetMeshIndex(),
					.modelScale    = model->GetModelScale()
				};

				memcpy(vertexBufferOffset + vertexModelOffset, &modelVertexData, vertexStrideSize);
			}

			// Pixel Data
			{
				const ModelPixelData modelPixelData
				{
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
	std::shared_ptr<ModelBundle>&& modelBundle, TemporaryDataBufferGPU&// Not needed in this system.
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
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, tempBuffer
	);
}

void ModelManagerVSIndividual::CopyOldBuffers(const D3DCommandList& copyList) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_indexBuffer.CopyOldBuffer(copyList);
		m_vertexBuffer.CopyOldBuffer(copyList);

		m_oldBufferCopyNecessary = false;
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
		const MeshManagerVertexShader& meshBundle = m_meshBundles.at(
			static_cast<size_t>(modelBundle.GetMeshBundleIndex())
		);

		meshBundle.Bind(graphicsList);

		// Model
		modelBundle.Draw(graphicsList, m_constantsRootIndex, meshBundle);
	}
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	ID3D12Device5* device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
	std::uint32_t frameCount
) : ModelManager{ device, memoryManager, frameCount },
	m_stagingBufferMan{ stagingBufferMan }, m_argumentInputBuffers{}, m_argumentOutputBuffers{},
	m_cullingDataBuffer{ device, memoryManager, D3D12_RESOURCE_STATE_GENERIC_READ },
	m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
	m_meshBundleIndexBuffer{ device, memoryManager, frameCount },
	m_vertexBuffer{ device, memoryManager },
	m_indexBuffer{ device, memoryManager },
	m_perModelDataBuffer{ device, memoryManager },
	m_perMeshDataBuffer{ device, memoryManager },
	m_perMeshBundleDataBuffer{ device, memoryManager },
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
	std::shared_ptr<ModelBundle>&& modelBundle, TemporaryDataBufferGPU& tempBuffer
) {
	ModelBundleCSIndirect modelBundleCS{};

	modelBundleCS.SetModelBundle(modelBundle);
	modelBundleObj.SetModelBundle(std::move(modelBundle));

	modelBundleCS.CreateBuffers(
		*m_stagingBufferMan, m_argumentInputBuffers, m_cullingDataBuffer, m_perModelDataBuffer,
		std::move(modelIndices), tempBuffer
	);

	modelBundleObj.CreateBuffers(
		m_argumentOutputBuffers, m_counterBuffers, modelBundleCS.GetModelCount()
	);

	UpdateCounterResetValues();

	modelBundleObj.SetID(static_cast<std::uint32_t>(modelBundleCS.GetID()));

	const auto modelBundleIndexInBuffer = modelBundleCS.GetModelBundleIndex();

	m_meshBundleIndexBuffer.Add(modelBundleIndexInBuffer);

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

				bundle.ResetCullingData();

				m_cullingDataBuffer.RelinquishMemory(bundle.GetCullingSharedData());
				m_perModelDataBuffer.RelinquishMemory(bundle.GetPerModelDataSharedData());

				const auto& modelIndices = bundle.GetModelIndices();

				for (const auto& modelIndex : modelIndices)
					m_modelBuffers.Remove(modelIndex);
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

		const SharedBufferData& perMeshSharedData = meshManager.GetPerMeshSharedData();
		m_perMeshDataBuffer.RelinquishMemory(perMeshSharedData);

		const SharedBufferData& perMeshBundleSharedData = meshManager.GetPerMeshBundleSharedData();
		m_perMeshBundleDataBuffer.RelinquishMemory(perMeshBundleSharedData);
	}
}

void ModelManagerVSIndirect::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, m_perMeshDataBuffer,
		m_perMeshBundleDataBuffer, tempBuffer
	);
}

void ModelManagerVSIndirect::_updatePerFrame(UINT64 frameIndex) const noexcept
{
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
		const std::uint32_t meshBundleIndex  = bundle.GetMeshBundleIndex();

		bundle.Update(
			static_cast<size_t>(frameIndex), m_meshBundles.at(static_cast<size_t>(meshBundleIndex))
		);

		const std::uint32_t modelBundleIndex = bundle.GetModelBundleIndex();

		bufferOffset = strideSize * modelBundleIndex;

		memcpy(bufferOffsetPtr + bufferOffset, &meshBundleIndex, strideSize);
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
			s_perModelDataSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_perMeshDataSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_perMeshBundleDataSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		);
		descriptorManager.AddRootSRV(
			s_meshBundleIndexSRVRegisterSlot, csRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
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
			s_perModelDataSRVRegisterSlot, csRegisterSpace,
			m_perModelDataBuffer.GetGPUAddress(), false
		);

		m_meshBundleIndexBuffer.SetRootSRVCom(
			descriptorManager, s_meshBundleIndexSRVRegisterSlot, csRegisterSpace
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorsCSOfMeshes(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t csRegisterSpace
) const {
	for (auto& descriptorManager : descriptorManagers)
	{
		descriptorManager.SetRootSRV(
			s_perMeshDataSRVRegisterSlot, csRegisterSpace, m_perMeshDataBuffer.GetGPUAddress(), false
		);
		descriptorManager.SetRootSRV(
			s_perMeshBundleDataSRVRegisterSlot, csRegisterSpace,
			m_perMeshBundleDataBuffer.GetGPUAddress(), false
		);
	}
}

void ModelManagerVSIndirect::Dispatch(const D3DCommandList& computeList) const noexcept
{
	ID3D12GraphicsCommandList* cmdList = computeList.Get();

	m_computePipeline.Bind(computeList);

	{
		constexpr auto pushConstantCount = GetConstantCount();

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
	size_t frameIndex, const D3DCommandList& graphicsList, ID3D12CommandSignature* commandSignature
) const noexcept {
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	GraphicsPipelineVertexShader::SetIATopology(graphicsList);

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsList, previousPSOIndex);

		// Mesh
		const MeshManagerVertexShader& meshBundle = m_meshBundles.at(
			static_cast<size_t>(modelBundle.GetMeshBundleIndex())
		);

		meshBundle.Bind(graphicsList);

		// Model
		modelBundle.Draw(frameIndex, commandSignature, graphicsList);
	}
}

void ModelManagerVSIndirect::CopyOldBuffers(const D3DCommandList& copyList) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_vertexBuffer.CopyOldBuffer(copyList);
		m_indexBuffer.CopyOldBuffer(copyList);
		m_perModelDataBuffer.CopyOldBuffer(copyList);
		m_perMeshDataBuffer.CopyOldBuffer(copyList);
		m_perMeshBundleDataBuffer.CopyOldBuffer(copyList);

		// I don't think copying is needed for the Output Argument
		// and the counter buffers. As their data will be only
		// needed on the same frame and not afterwards.

		m_oldBufferCopyNecessary = false;
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
	m_constantsRootIndex{ 0u },
	m_stagingBufferMan{ stagingBufferMan },
	m_perMeshletBuffer{ device, memoryManager },
	m_vertexBuffer{ device, memoryManager },
	m_vertexIndicesBuffer{ device, memoryManager },
	m_primIndicesBuffer{ device, memoryManager }
{}

void ModelManagerMS::ConfigureModelBundle(
	ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundle>&& modelBundle, [[maybe_unused]] TemporaryDataBufferGPU& tempBuffer
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

		const SharedBufferData& perMeshletSharedData = meshManager.GetPerMeshletSharedData();
		m_perMeshletBuffer.RelinquishMemory(perMeshletSharedData);
	}
}

void ModelManagerMS::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerMeshShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_vertexIndicesBuffer,
		m_primIndicesBuffer, m_perMeshletBuffer, tempBuffer
	);
}

void ModelManagerMS::_setGraphicsConstantRootIndex(
	const D3DDescriptorManager& descriptorManager, size_t constantsRegisterSpace
) noexcept {
	m_constantsRootIndex = descriptorManager.GetRootIndexCBV(
		s_constantDataCBVRegisterSlot, constantsRegisterSpace
	);
}

void ModelManagerMS::CopyOldBuffers(const D3DCommandList& copyList) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_perMeshletBuffer.CopyOldBuffer(copyList);
		m_vertexBuffer.CopyOldBuffer(copyList);
		m_vertexIndicesBuffer.CopyOldBuffer(copyList);
		m_primIndicesBuffer.CopyOldBuffer(copyList);

		m_oldBufferCopyNecessary = false;
	}
}

void ModelManagerMS::SetDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t msRegisterSpace,
	size_t psRegisterSpace
) const noexcept {
	const auto frameCount             = std::size(descriptorManagers);
	constexpr UINT meshConstantCount  = MeshManagerMeshShader::GetConstantCount();
	constexpr UINT modelConstantCount = ModelBundleMSIndividual::GetConstantCount();

	for (size_t index = 0u; index < frameCount; ++index)
	{
		D3DDescriptorManager& descriptorManager = descriptorManagers[index];

		descriptorManager.AddConstants(
			s_constantDataCBVRegisterSlot, msRegisterSpace, meshConstantCount + modelConstantCount,
			D3D12_SHADER_VISIBILITY_ALL
		); // Both the AS and MS will use it.
		descriptorManager.AddRootSRV(
			s_modelBuffersGraphicsSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		); // Both the AS and MS will use it.
		descriptorManager.AddRootSRV(
			s_modelBuffersPixelSRVRegisterSlot, psRegisterSpace, D3D12_SHADER_VISIBILITY_PIXEL
		);
		descriptorManager.AddRootSRV(
			s_perMeshletBufferSRVRegisterSlot, msRegisterSpace, D3D12_SHADER_VISIBILITY_ALL
		); // Both the AS and MS will use it.
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
			s_perMeshletBufferSRVRegisterSlot, msRegisterSpace, m_perMeshletBuffer.GetGPUAddress(), true
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

		const auto meshBundleIndex              = static_cast<size_t>(
			modelBundle.GetMeshBundleIndex()
		);
		const MeshManagerMeshShader& meshBundle = m_meshBundles.at(meshBundleIndex);

		constexpr UINT constBufferCount  = MeshManagerMeshShader::GetConstantCount();

		const MeshManagerMeshShader::MeshBundleDetailsMS meshBundleDetailsMS
			= meshBundle.GetMeshBundleDetailsMS();

		cmdList->SetGraphicsRoot32BitConstants(
			m_constantsRootIndex, constBufferCount, &meshBundleDetailsMS, 0u
		);

		// Model
		modelBundle.Draw(graphicsList, m_constantsRootIndex, meshBundle);
	}
}
