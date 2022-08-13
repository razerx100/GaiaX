#include <D3DDescriptorView.hpp>
#include <D3DHelperFunctions.hpp>

// D3D Root Descriptor View
void D3DRootDescriptorView::SetAddressesStart(
	size_t addressesStart, size_t subAllocationSize, size_t alignment
) noexcept {
	m_cpuAddress.SetAddress(addressesStart, subAllocationSize, alignment);
	m_gpuAddress.SetAddress(addressesStart, subAllocationSize, alignment);
}

void D3DRootDescriptorView::UpdateCPUAddressStart(std::uint8_t* offset) noexcept {
	m_cpuAddress.UpdateAddressStart(offset);
}

void D3DRootDescriptorView::UpdateGPUAddressStart(D3D12_GPU_VIRTUAL_ADDRESS offset) noexcept {
	m_gpuAddress.UpdateAddressStart(offset);
}

std::uint8_t* D3DRootDescriptorView::GetCPUAddressStart(size_t index) const noexcept {
	return m_cpuAddress.GetAddressPTRStart<std::uint8_t>(index);
}

D3D12_GPU_VIRTUAL_ADDRESS D3DRootDescriptorView::GetGPUAddressStart(
	size_t index
) const noexcept {
	return m_gpuAddress.GetAddressStart<D3D12_GPU_VIRTUAL_ADDRESS>(index);
}

// D3D Descriptor View
D3DDescriptorView::D3DDescriptorView(ResourceType type, D3D12_RESOURCE_FLAGS flags)
	: m_resourceBuffer{ type, flags }, m_gpuHandleStart{}, m_cpuHandleStart{},
	m_descriptorSize{ 0u },
	m_isUAV{ flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS }, m_subAllocationSize{ 0u },
	m_strideSize{ 0u }{}

void D3DDescriptorView::SetDescriptorHandles(
	SharedCPUHandle cpuHandle, SharedGPUHandle gpuHandle, size_t descriptorSize
) noexcept {
	m_sharedCPUHandle = cpuHandle;
	m_sharedGPUHandle = gpuHandle;
	m_descriptorSize = descriptorSize;
}

void D3DDescriptorView::SetBufferInfo(
	ID3D12Device* device,
	UINT strideSize, UINT elementsPerAllocation, size_t subAllocationCount
) noexcept {
	size_t bufferSize = 0u;
	m_strideSize = strideSize;

	D3D12_BUFFER_UAV bufferInfo{};
	bufferInfo.StructureByteStride = strideSize;
	bufferInfo.NumElements = elementsPerAllocation;

	size_t bufferSizePerAllocation = static_cast<size_t>(strideSize) * elementsPerAllocation;

	if (m_isUAV) {
		UINT64 counterOffset = 0u;
		UINT64 firstElement = 1u;
		UINT64 alignedSubAllocationSize = Align(
			strideSize + bufferSizePerAllocation, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT
		);

		for (size_t index = 0u; index < subAllocationCount; ++index) {
			bufferInfo.CounterOffsetInBytes = counterOffset;
			bufferInfo.FirstElement = firstElement;

			m_bufferInfos.emplace_back(bufferInfo);

			bufferSize += alignedSubAllocationSize + sizeof(UINT);

			counterOffset += alignedSubAllocationSize;
			firstElement = Align(counterOffset + sizeof(UINT), strideSize) / strideSize;
		}

		m_subAllocationSize = alignedSubAllocationSize;
	}
	else {
		for (size_t index = 0u; index < subAllocationCount; ++index) {
			bufferInfo.FirstElement = index * elementsPerAllocation;

			m_bufferInfos.emplace_back(bufferInfo);

			bufferSize += bufferSizePerAllocation;
		}

		m_subAllocationSize = bufferSizePerAllocation;
	}

	m_resourceBuffer.SetBufferInfo(bufferSize);
	m_resourceBuffer.ReserveHeapSpace(device);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DDescriptorView::GetCPUDescriptorHandle(
	size_t index
) const noexcept {
	return D3D12_CPU_DESCRIPTOR_HANDLE{ m_cpuHandleStart.ptr + (m_descriptorSize * index) };
}

D3D12_GPU_DESCRIPTOR_HANDLE D3DDescriptorView::GetGPUDescriptorHandle(
	size_t index
) const noexcept {
	return D3D12_GPU_DESCRIPTOR_HANDLE{ m_gpuHandleStart.ptr + (m_descriptorSize * index) };
}

UINT64 D3DDescriptorView::GetCounterOffset(size_t index) const noexcept {
	return static_cast<UINT64>(index * m_subAllocationSize);
}

std::uint8_t* D3DDescriptorView::GetCPUWPointer(size_t index) const noexcept {
	return m_resourceBuffer.GetCPUWPointer()
		+ index * m_subAllocationSize + m_isUAV * m_strideSize;
}

void D3DDescriptorView::CreateDescriptorView(ID3D12Device* device) {
	m_cpuHandleStart.ptr = *m_sharedCPUHandle;
	m_gpuHandleStart.ptr = *m_sharedGPUHandle;

	m_resourceBuffer.CreateResource(device, D3D12_RESOURCE_STATE_COPY_DEST);

	if (m_isUAV) {
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
		desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_UNKNOWN;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_cpuHandleStart;

		for (auto& bufferInfo : m_bufferInfos) {
			desc.Buffer = bufferInfo;

			device->CreateUnorderedAccessView(
				m_resourceBuffer.GetResource(), m_resourceBuffer.GetResource(),
				&desc, cpuHandle
			);

			cpuHandle.ptr += m_descriptorSize;
		}
	}
	else {
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_cpuHandleStart;

		for (auto& bufferInfo : m_bufferInfos) {
			desc.Buffer.FirstElement = bufferInfo.FirstElement;
			desc.Buffer.NumElements = bufferInfo.NumElements;
			desc.Buffer.StructureByteStride = bufferInfo.StructureByteStride;

			device->CreateShaderResourceView(
				m_resourceBuffer.GetResource(), &desc, cpuHandle
			);

			cpuHandle.ptr += m_descriptorSize;
		}
	}
}
