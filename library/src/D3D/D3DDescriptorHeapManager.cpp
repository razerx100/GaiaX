#include <D3DDescriptorHeapManager.hpp>
#include <cassert>

D3DDescriptorHeap::D3DDescriptorHeap(
    ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag
) : m_descriptorHeap{},
    m_descriptorSize{ device->GetDescriptorHandleIncrementSize(type) },
    m_descriptorDesc{ .Type = type, .Flags = flag }, m_device{ device }
{}

void D3DDescriptorHeap::Create(UINT descriptorCount)
{
    const UINT oldDescriptorCount   = m_descriptorDesc.NumDescriptors;
    m_descriptorDesc.NumDescriptors = descriptorCount;

    ComPtr<ID3D12DescriptorHeap> newDescriptorHeap{};

    m_device->CreateDescriptorHeap(&m_descriptorDesc, IID_PPV_ARGS(&newDescriptorHeap));

    if (m_descriptorHeap && oldDescriptorCount <= descriptorCount)
        m_device->CopyDescriptorsSimple(
            oldDescriptorCount,
            newDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            m_descriptorDesc.Type
        );

    m_descriptorHeap = std::move(newDescriptorHeap);
}

void D3DDescriptorHeap::CopyHeap(
    const D3DDescriptorHeap& src, UINT descriptorCount, UINT srcOffset, UINT dstOffset
) {
    assert(src.m_descriptorDesc.Type == m_descriptorDesc.Type && "Descriptor heaps have different types.");
    assert(
        src.m_descriptorDesc.NumDescriptors >= srcOffset + descriptorCount
        && "Src descriptors out of bound."
    );
    assert(
        m_descriptorDesc.NumDescriptors >= dstOffset + descriptorCount && "Dst descriptors out of bound."
    );

    m_device->CopyDescriptorsSimple(
        descriptorCount, GetCPUHandle(dstOffset), src.GetCPUHandle(srcOffset), m_descriptorDesc.Type
    );
}

void D3DDescriptorHeap::Bind(ID3D12GraphicsCommandList* commandList) const noexcept
{
	commandList->SetDescriptorHeaps(1u, m_descriptorHeap.GetAddressOf());
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DDescriptorHeap::GetCPUHandle(UINT index) const noexcept
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr                        += static_cast<SIZE_T>(index) * m_descriptorSize;

    return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3DDescriptorHeap::GetGPUHandle(UINT index) const noexcept
{
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr                        += static_cast<UINT64>(index) * m_descriptorSize;

    return gpuHandle;
}

// D3D Reusable Descriptor Heap
UINT D3DReusableDescriptorHeap::CreateSRV(
    ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc
) {
    assert(
        m_descriptorHeap.GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        && "The heap type isn't CBV_SRV_UAV."
    );

    const UINT descriptorIndex                  = AllocateDescriptor();
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap.GetCPUHandle(descriptorIndex);

    m_device->CreateShaderResourceView(resource, &srvDesc, cpuHandle);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc)
{
    assert(
        m_descriptorHeap.GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        && "The heap type isn't CBV_SRV_UAV."
    );

    const UINT descriptorIndex                  = AllocateDescriptor();
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap.GetCPUHandle(descriptorIndex);

    m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateUAV(
    ID3D12Resource* resource, ID3D12Resource* counterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc
) {
    assert(
        m_descriptorHeap.GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        && "The heap type isn't CBV_SRV_UAV."
    );

    const UINT descriptorIndex                  = AllocateDescriptor();
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap.GetCPUHandle(descriptorIndex);

    m_device->CreateUnorderedAccessView(resource, counterResource, &uavDesc, cpuHandle);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateDSV(
    ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc
) {
    assert(
        m_descriptorHeap.GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_DSV
        && "The heap type isn't DSV."
    );

    const UINT descriptorIndex                  = AllocateDescriptor();
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap.GetCPUHandle(descriptorIndex);

    m_device->CreateDepthStencilView(resource, &dsvDesc, cpuHandle);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateRTV(
    ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc
) {
    assert(
        m_descriptorHeap.GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_RTV
        && "The heap type isn't RTV."
    );

    const UINT descriptorIndex                  = AllocateDescriptor();
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap.GetCPUHandle(descriptorIndex);

    m_device->CreateRenderTargetView(resource, &rtvDesc, cpuHandle);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc)
{
    assert(
        m_descriptorHeap.GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        && "The heap type isn't Sampler."
    );

    const UINT descriptorIndex                  = AllocateDescriptor();
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeap.GetCPUHandle(descriptorIndex);

    m_device->CreateSampler(&samplerDesc, cpuHandle);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::GetNextFreeIndex(UINT extraAllocCount/* = 0 */)
{
    UINT descriptorIndex  = std::numeric_limits<UINT>::max();
    auto oDescriptorIndex = m_indicesManager.GetFirstAvailableIndex();

    if (oDescriptorIndex)
        descriptorIndex = static_cast<UINT>(oDescriptorIndex.value());
    else
    {
        // This part should only be executed when both the availableIndices and elements
        // containers have the same size. So, getting the size should be fine.
        descriptorIndex = m_descriptorHeap.GetDescriptorCount();

        // ElementIndex is the previous size, we have the new item, and then the extraAllocations.
        const UINT newDescriptorCount = descriptorIndex + 1u + extraAllocCount;

        ReserveNewElements(newDescriptorCount);
    }

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::AllocateDescriptor()
{
    const UINT descriptorIndex = GetNextFreeIndex(s_extraAllocationCount);
    m_indicesManager.ToggleAvailability(descriptorIndex, false);

    return descriptorIndex;
}

void D3DReusableDescriptorHeap::ReserveNewElements(UINT newDescriptorCount)
{
    m_indicesManager.Resize(newDescriptorCount);
    m_descriptorHeap.Create(newDescriptorCount);
}

// D3D Descriptor Layout
void D3DDescriptorMap::Bind(
	const D3DDescriptorHeap& descriptorHeap, ID3D12GraphicsCommandList* commandList
) const {
	for (const auto& viewMap : m_singleDescriptors)
		viewMap.bindViewFunction(commandList, viewMap.rootIndex, viewMap.bufferAddress);

	for (const auto& tableMap : m_descriptorTables)
		tableMap.bindTableFunction(
			commandList, tableMap.rootIndex, descriptorHeap.GetGPUHandle(tableMap.descriptorIndex)
		);
}

D3DDescriptorMap& D3DDescriptorMap::AddCBVGfx(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddCBVCom(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetComputeRootConstantBufferView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddUAVGfx(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetGraphicsRootUnorderedAccessView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddUAVCom(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetComputeRootUnorderedAccessView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddSRVGfx(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetGraphicsRootShaderResourceView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddSRVCom(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
	m_singleDescriptors.emplace_back(
		SingleDescriptorMap{
			.rootIndex        = rootIndex,
			.bufferAddress    = bufferAddress,
			.bindViewFunction = &ProxyView<&ID3D12GraphicsCommandList::SetComputeRootShaderResourceView>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddDescTableGfx(UINT rootIndex, UINT descriptorIndex) noexcept
{
	m_descriptorTables.emplace_back(
		DescriptorTableMap{
			.rootIndex         = rootIndex,
			.descriptorIndex   = descriptorIndex,
			.bindTableFunction = &ProxyTable<&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable>
		}
	);

	return *this;
}

D3DDescriptorMap& D3DDescriptorMap::AddDescTableCom(UINT rootIndex, UINT descriptorIndex) noexcept
{
	m_descriptorTables.emplace_back(
		DescriptorTableMap{
			.rootIndex         = rootIndex,
			.descriptorIndex   = descriptorIndex,
			.bindTableFunction = &ProxyTable<&ID3D12GraphicsCommandList::SetComputeRootDescriptorTable>
		}
	);

	return *this;
}
