#include <ComputePipelineIndirectDraw.hpp>
#include <Shader.hpp>
#include <D3DResourceBarrier.hpp>
#include <cmath>
#include <Gaia.hpp>

ComputePipelineIndirectDraw::ComputePipelineIndirectDraw(std::uint32_t frameCount)
	: m_argumentBufferSRV{ DescriptorType::SRV },
	m_argumentBufferUAVs{ frameCount, { ResourceType::gpuOnly, DescriptorType::UAV } },
	m_counterBuffers{ frameCount, DescriptorType::UAV },
	m_counterResetBuffer{ ResourceType::cpuWrite }, m_modelCount{ 0u },
	m_frameCount{ frameCount } {}

void ComputePipelineIndirectDraw::BindComputePipeline(
	ID3D12GraphicsCommandList* computeCommandList
) const noexcept {
	computeCommandList->SetPipelineState(m_computePSO->Get());
	computeCommandList->SetComputeRootSignature(m_computeRS->Get());
}

void ComputePipelineIndirectDraw::CreateComputeRootSignature(ID3D12Device* device) noexcept {
	auto computeRS = _createComputeRootSignature(device);
	m_computeRSLayout = computeRS->GetElementLayout();

	m_computeRS = std::move(computeRS);
}

void ComputePipelineIndirectDraw::CreateComputePipelineObject(
	ID3D12Device* device, const std::wstring& shaderPath
) noexcept {
	m_computePSO = _createComputePipelineObject(device, m_computeRS->Get(), shaderPath);
}

void ComputePipelineIndirectDraw::DispatchCompute(
	ID3D12GraphicsCommandList* computeCommandList, size_t frameIndex
) const noexcept {
	static constexpr size_t argumentBufferSRVIndex =
		static_cast<size_t>(RootSigElement::IndirectArgsSRV);
	static constexpr size_t argumentBufferUAVIndex =
		static_cast<size_t>(RootSigElement::IndirectArgsUAV);
	static constexpr size_t counterBufferIndex =
		static_cast<size_t>(RootSigElement::IndirectArgsCounterUAV);
	static constexpr size_t cullingDataIndex =
		static_cast<size_t>(RootSigElement::CullingData);

	computeCommandList->SetComputeRootDescriptorTable(
		m_computeRSLayout[argumentBufferSRVIndex],
		m_argumentBufferSRV.GetFirstGPUDescriptorHandle()
	);
	computeCommandList->SetComputeRootDescriptorTable(
		m_computeRSLayout[argumentBufferUAVIndex],
		m_argumentBufferUAVs[frameIndex].GetFirstGPUDescriptorHandle()
	);
	computeCommandList->SetComputeRootDescriptorTable(
		m_computeRSLayout[counterBufferIndex],
		m_counterBuffers[frameIndex].GetFirstGPUDescriptorHandle()
	);
	computeCommandList->SetComputeRootConstantBufferView(
		m_computeRSLayout[cullingDataIndex], m_cullingDataBuffer.GetFirstGPUAddress()
	);

	computeCommandList->Dispatch(
		static_cast<UINT>(std::ceil(m_modelCount / THREADBLOCKSIZE)), 1u, 1u
	);
}

void ComputePipelineIndirectDraw::CreateBuffers(ID3D12Device* device) {
	const D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart =
		Gaia::descriptorTable->GetUploadDescriptorStart();

	const D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart =
		Gaia::descriptorTable->GetGPUDescriptorStart();

	m_argumentBufferSRV.CreateDescriptorView(
		device, uploadDescriptorStart, gpuDescriptorStart, D3D12_RESOURCE_STATE_COPY_DEST
	);

	for (auto& argumentBufferUAV : m_argumentBufferUAVs)
		argumentBufferUAV.CreateDescriptorView(
			device, uploadDescriptorStart, gpuDescriptorStart,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

	for (auto& counterBuffer : m_counterBuffers)
		counterBuffer.CreateDescriptorView(
			device, uploadDescriptorStart, gpuDescriptorStart, D3D12_RESOURCE_STATE_COPY_DEST
		);

	m_counterResetBuffer.CreateResource(device, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_cullingDataBuffer.CreateResource(device);

	// copy the culling data to the buffer.
	std::uint8_t* cullingBufferPtr = m_cullingDataBuffer.GetFirstCPUWPointer();

	CullingData cullingData{};
	cullingData.modelCount = static_cast<std::uint32_t>(std::size(m_indirectArguments));
	cullingData.modelTypes = static_cast<std::uint32_t>(std::size(m_modelCountOffsets));
	cullingData.xBounds = XBOUNDS;
	cullingData.yBounds = YBOUNDS;
	cullingData.zBounds = ZBOUNDS;

	memcpy(cullingBufferPtr, &cullingData, sizeof(CullingData));

	// Copy modelCount offsets
	for (auto& counterBuffer : m_counterBuffers) {
		std::uint8_t* offsetBufferPtr = counterBuffer.GetFirstCPUWPointer();

		struct {
			std::uint32_t counter;
			std::uint32_t modelCountOffset;
		}sourceCountOffset{ 0u, 0u };
		size_t destOffset = 0u;

		for (auto modelCountOffset : m_modelCountOffsets) {
			sourceCountOffset.modelCountOffset = modelCountOffset;

			memcpy(
				offsetBufferPtr + destOffset, &sourceCountOffset,
				COUNTERBUFFERSTRIDE
			);

			destOffset += COUNTERBUFFERSTRIDE;
		}
	}

	// copy zero to counter buffer
	std::uint8_t* counterCPUPtr = m_counterResetBuffer.GetFirstCPUWPointer();
	const UINT zeroValue = 0u;
	memcpy(counterCPUPtr, &zeroValue, sizeof(UINT));

	// Copy Indirect Arguments
	std::uint8_t* argumentCPUPtr = m_argumentBufferSRV.GetFirstCPUWPointer();
	memcpy(
		argumentCPUPtr, std::data(m_indirectArguments),
		sizeof(IndirectArguments) * std::size(m_indirectArguments)
	);

	m_indirectArguments = std::vector<IndirectArguments>();
}

void ComputePipelineIndirectDraw::ReserveBuffers(ID3D12Device* device) {
	const size_t argumentDescriptorOffsetSRV =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset();
	size_t argumentDescriptorOffsetUAV =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);
	size_t counterDescriptorOffset =
		Gaia::descriptorTable->ReserveDescriptorsAndGetOffset(m_frameCount);

	const UINT descriptorSize =
		device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	static constexpr auto indirectStructSize = static_cast<UINT>(sizeof(IndirectArguments));

	m_argumentBufferSRV.SetDescriptorOffset(argumentDescriptorOffsetSRV, descriptorSize);
	m_argumentBufferSRV.SetBufferInfo(device, indirectStructSize, m_modelCount);

	for (auto& argumentBufferUAV : m_argumentBufferUAVs) {
		argumentBufferUAV.SetDescriptorOffset(argumentDescriptorOffsetUAV, descriptorSize);
		argumentBufferUAV.SetBufferInfo(device, indirectStructSize, m_modelCount);

		++argumentDescriptorOffsetUAV;
	}

	const auto counterCount = static_cast<UINT>(std::size(m_modelCountOffsets));

	for (auto& counterBuffer : m_counterBuffers) {
		counterBuffer.SetDescriptorOffset(counterDescriptorOffset, descriptorSize);
		counterBuffer.SetBufferInfo(device, COUNTERBUFFERSTRIDE, counterCount);

		++counterDescriptorOffset;
	}

	m_counterResetBuffer.SetBufferInfo(sizeof(UINT));
	m_counterResetBuffer.ReserveHeapSpace(device);

	m_cullingDataBuffer.SetBufferInfo(sizeof(CullingData));
	m_cullingDataBuffer.ReserveHeapSpace(device);
}

void ComputePipelineIndirectDraw::RecordIndirectArguments(
	const std::vector<std::shared_ptr<IModel>>& models
) noexcept {
	for (size_t index = 0u; index < std::size(models); ++index) {
		IndirectArguments arguments{};
		arguments.modelIndex = static_cast<std::uint32_t>(std::size(m_indirectArguments));

		const auto& model = models[index];

		arguments.drawIndexed.BaseVertexLocation = 0u;
		arguments.drawIndexed.IndexCountPerInstance = model->GetIndexCount();
		arguments.drawIndexed.StartIndexLocation = model->GetIndexOffset();
		arguments.drawIndexed.InstanceCount = 1u;
		arguments.drawIndexed.StartInstanceLocation = 0u;

		m_indirectArguments.emplace_back(arguments);
	}

	m_modelCountOffsets.emplace_back(m_modelCount);
	m_modelCount += static_cast<UINT>(std::size(models));
}

void ComputePipelineIndirectDraw::ResetCounterBuffer(
	ID3D12GraphicsCommandList* commandList, size_t frameIndex
) const noexcept {
	ID3D12Resource* counterBuffer = m_counterBuffers[frameIndex].GetResource();

	for (UINT64 index = 0u; index < std::size(m_modelCountOffsets); ++index)
		commandList->CopyBufferRegion(
			counterBuffer, COUNTERBUFFERSTRIDE * index,
			m_counterResetBuffer.GetResource(),
			m_counterResetBuffer.GetFirstSubAllocationOffset(),
			static_cast<UINT64>(sizeof(UINT))
		); // UAV getting promoted from COMMON to COPY_DEST

	// So need to change it back to UNORDERED_ACCESS
	D3DResourceBarrier().AddBarrier(
		counterBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	).RecordBarriers(commandList);
}

void ComputePipelineIndirectDraw::RecordResourceUpload(
	ID3D12GraphicsCommandList* copyList
) noexcept {
	m_argumentBufferSRV.RecordResourceUpload(copyList);
	m_cullingDataBuffer.RecordResourceUpload(copyList);

	for (auto& counterBuffer : m_counterBuffers)
		counterBuffer.RecordResourceUpload(copyList);
}

void ComputePipelineIndirectDraw::ReleaseUploadResource() noexcept {
	m_argumentBufferSRV.ReleaseUploadResource();
	m_cullingDataBuffer.ReleaseUploadResource();

	for (auto& counterBuffer : m_counterBuffers)
		counterBuffer.ReleaseUploadResource();
}

std::unique_ptr<RootSignatureDynamic> ComputePipelineIndirectDraw::_createComputeRootSignature(
	ID3D12Device* device
) const noexcept {
	auto signature = std::make_unique<RootSignatureDynamic>();

	signature->AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::ModelData, false, 0u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::IndirectArgsSRV, false, 1u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::IndirectArgsUAV, false, 0u
	).AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, D3D12_SHADER_VISIBILITY_ALL,
		RootSigElement::IndirectArgsCounterUAV, false, 1u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_ALL, RootSigElement::Camera, 0u
	).AddConstantBufferView(
		D3D12_SHADER_VISIBILITY_ALL, RootSigElement::CullingData, 1u
	).CompileSignature().CreateSignature(device);

	return signature;
}

std::unique_ptr<D3DPipelineObject> ComputePipelineIndirectDraw::_createComputePipelineObject(
	ID3D12Device* device, ID3D12RootSignature* computeRootSignature,
	const std::wstring& shaderPath
) const noexcept {
	auto cs = std::make_unique<Shader>();
	cs->LoadBinary(shaderPath + L"ComputeShader.cso");

	auto pso = std::make_unique<D3DPipelineObject>();
	pso->CreateComputePipelineState(device, computeRootSignature, cs->GetByteCode());

	return pso;
}

RSLayoutType ComputePipelineIndirectDraw::GetComputeRSLayout() const noexcept {
	return m_computeRSLayout;
}

ID3D12Resource* ComputePipelineIndirectDraw::GetArgumentBuffer(
	size_t frameIndex
) const noexcept {
	return m_argumentBufferUAVs[frameIndex].GetResource();
}

ID3D12Resource* ComputePipelineIndirectDraw::GetCounterBuffer(
	size_t frameIndex
) const noexcept {
	return m_counterBuffers[frameIndex].GetResource();
}

UINT ComputePipelineIndirectDraw::GetCurrentModelCount() const noexcept {
	return m_modelCount;
}

size_t ComputePipelineIndirectDraw::GetCounterCount() const noexcept {
	return std::size(m_modelCountOffsets);
}
