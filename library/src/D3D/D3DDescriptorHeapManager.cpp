#include <D3DDescriptorHeapManager.hpp>
#include <ranges>
#include <algorithm>
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

void D3DDescriptorHeap::CopyDescriptors(
    const D3DDescriptorHeap& src, UINT descriptorCount, UINT srcOffset, UINT dstOffset
) const {
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

void D3DDescriptorHeap::CreateSRV(
    ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, UINT descriptorIndex
) const {
    assert(
        GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        && "The heap type isn't CBV_SRV_UAV."
    );

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(descriptorIndex);

    m_device->CreateShaderResourceView(resource, &srvDesc, cpuHandle);
}

void D3DDescriptorHeap::CreateCBV(
    const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc, UINT descriptorIndex
) const {
    assert(
        GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        && "The heap type isn't CBV_SRV_UAV."
    );

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(descriptorIndex);

    m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);
}

void D3DDescriptorHeap::CreateUAV(
    ID3D12Resource* resource, ID3D12Resource* counterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT descriptorIndex
) const {
    assert(
        GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        && "The heap type isn't CBV_SRV_UAV."
    );

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(descriptorIndex);

    m_device->CreateUnorderedAccessView(resource, counterResource, &uavDesc, cpuHandle);
}

void D3DDescriptorHeap::CreateDSV(
    ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, UINT descriptorIndex
) const {
    assert(
        GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_DSV
        && "The heap type isn't DSV."
    );

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(descriptorIndex);

    m_device->CreateDepthStencilView(resource, &dsvDesc, cpuHandle);
}

void D3DDescriptorHeap::CreateRTV(
    ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, UINT descriptorIndex
) const {
    assert(
        GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_RTV
        && "The heap type isn't RTV."
    );

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(descriptorIndex);

    m_device->CreateRenderTargetView(resource, &rtvDesc, cpuHandle);
}

void D3DDescriptorHeap::CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc, UINT descriptorIndex) const
{
    assert(
        GetHeapType() == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        && "The heap type isn't Sampler."
    );

    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCPUHandle(descriptorIndex);

    m_device->CreateSampler(&samplerDesc, cpuHandle);
}

// D3D Reusable Descriptor Heap
UINT D3DReusableDescriptorHeap::CreateSRV(
    ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc
) {
    const UINT descriptorIndex = AllocateDescriptor();

    m_descriptorHeap.CreateSRV(resource, srvDesc, descriptorIndex);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc)
{
    const UINT descriptorIndex = AllocateDescriptor();

    m_descriptorHeap.CreateCBV(cbvDesc, descriptorIndex);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateUAV(
    ID3D12Resource* resource, ID3D12Resource* counterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc
) {
    const UINT descriptorIndex = AllocateDescriptor();

    m_descriptorHeap.CreateUAV(resource, counterResource, uavDesc, descriptorIndex);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateDSV(
    ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc
) {
    const UINT descriptorIndex = AllocateDescriptor();

    m_descriptorHeap.CreateDSV(resource, dsvDesc, descriptorIndex);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateRTV(
    ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc
) {
    const UINT descriptorIndex = AllocateDescriptor();

    m_descriptorHeap.CreateRTV(resource, rtvDesc, descriptorIndex);

    return descriptorIndex;
}

UINT D3DReusableDescriptorHeap::CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc)
{
    const UINT descriptorIndex = AllocateDescriptor();

    m_descriptorHeap.CreateSampler(samplerDesc, descriptorIndex);

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

// D3D Descriptor Map
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

// D3D Descriptor Manager
void D3DDescriptorManager::CreateDescriptors(bool graphicsQueue)
{
    UINT totalDescriptorCount = 0u;

    for (size_t index = 0u; index < std::size(m_descriptorLayouts); ++index)
    {
        const D3DDescriptorLayout& layout = m_descriptorLayouts[index];
        const size_t detailsCount         = layout.GetDescriptorDetailsCount();
        totalDescriptorCount             += layout.GetTotalDescriptorCount();

        for (size_t detailsIndex = 0u; detailsIndex < detailsCount; ++detailsIndex)
        {
            D3DDescriptorLayout::DescriptorDetails details = layout.GetDescriptorDetails(detailsIndex);

            if (details.descriptorCount > 1u)
            {
                if (!graphicsQueue)
                    m_descriptorMap.AddDescTableCom(
                        GetRootIndex(detailsIndex, index), GetSlotOffset(detailsIndex, index)
                    );
                else
                    m_descriptorMap.AddDescTableGfx(
                        GetRootIndex(detailsIndex, index), GetSlotOffset(detailsIndex, index)
                    );
            }
        }
    }

    m_resourceHeapCPU.Create(totalDescriptorCount);
    m_resourceHeapGPU.Create(totalDescriptorCount);
}

void D3DDescriptorManager::Bind(ID3D12GraphicsCommandList* commandList) const noexcept
{
    // Bind the heap.
    m_resourceHeapGPU.Bind(commandList);

    // Now bind all the descriptors.
    m_descriptorMap.Bind(m_resourceHeapGPU, commandList);
}

void D3DDescriptorManager::SetCBV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex,
    ID3D12Resource* resource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc,
    bool graphicsQueue
) {
    const UINT descriptorIndexInHeap = GetDescriptorOffset(registerSlot, registerSpace, descriptorIndex);

    m_resourceHeapCPU.CreateCBV(cbvDesc, descriptorIndexInHeap);

    m_resourceHeapGPU.CopyDescriptors(
        m_resourceHeapCPU, 1u, descriptorIndexInHeap, descriptorIndexInHeap
    );

    {
        const D3DDescriptorLayout::DescriptorDetails details
            = m_descriptorLayouts[registerSpace].GetDescriptorDetails(registerSlot);

        // Only add the descriptor to the descriptor map, if the slot wasn't created for
        // multiple descriptors or a table.
        if (details.descriptorCount == 1u)
        {
            if (!graphicsQueue)
                m_descriptorMap.AddCBVCom(
                    GetRootIndex(registerSlot, registerSpace), resource->GetGPUVirtualAddress()
                );
            else
                m_descriptorMap.AddCBVGfx(
                    GetRootIndex(registerSlot, registerSpace), resource->GetGPUVirtualAddress()
                );
        }
    }
}

void D3DDescriptorManager::SetSRV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex,
    ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, bool graphicsQueue
) {
    const UINT descriptorIndexInHeap = GetDescriptorOffset(registerSlot, registerSpace, descriptorIndex);

    m_resourceHeapCPU.CreateSRV(resource, srvDesc, descriptorIndexInHeap);

    m_resourceHeapGPU.CopyDescriptors(
        m_resourceHeapCPU, 1u, descriptorIndexInHeap, descriptorIndexInHeap
    );

    {
        const D3DDescriptorLayout::DescriptorDetails details
            = m_descriptorLayouts[registerSpace].GetDescriptorDetails(registerSlot);

        // Only add the descriptor to the descriptor map, if the slot wasn't created for
        // multiple descriptors or a table.
        if (details.descriptorCount == 1u)
        {
            if (!graphicsQueue)
                m_descriptorMap.AddSRVCom(
                    GetRootIndex(registerSlot, registerSpace), resource->GetGPUVirtualAddress()
                );
            else
                m_descriptorMap.AddSRVGfx(
                    GetRootIndex(registerSlot, registerSpace), resource->GetGPUVirtualAddress()
                );
        }
    }
}

void D3DDescriptorManager::SetUAV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex,
    ID3D12Resource* resource, ID3D12Resource* counterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, bool graphicsQueue
) {
    const UINT descriptorIndexInHeap = GetDescriptorOffset(registerSlot, registerSpace, descriptorIndex);

    m_resourceHeapCPU.CreateUAV(resource, counterResource, uavDesc, descriptorIndexInHeap);

    m_resourceHeapGPU.CopyDescriptors(
        m_resourceHeapCPU, 1u, descriptorIndexInHeap, descriptorIndexInHeap
    );

    {
        const D3DDescriptorLayout::DescriptorDetails details
            = m_descriptorLayouts[registerSpace].GetDescriptorDetails(registerSlot);

        // Only add the descriptor to the descriptor map, if the slot wasn't created for
        // multiple descriptors or a table.
        // Dunno how the counter buffer gets bound. Maybe I will need to make these into tables?
        if (details.descriptorCount == 1u)
        {
            if (!graphicsQueue)
                m_descriptorMap.AddUAVCom(
                    GetRootIndex(registerSlot, registerSpace), resource->GetGPUVirtualAddress()
                );
            else
                m_descriptorMap.AddUAVGfx(
                    GetRootIndex(registerSlot, registerSpace), resource->GetGPUVirtualAddress()
                );
        }
    }
}

UINT D3DDescriptorManager::GetLayoutOffset(size_t layoutIndex) const noexcept
{
    UINT layoutOffset = 0u;

    for (size_t index = 0u; index < std::size(m_descriptorLayouts); ++index)
    {
        if (layoutIndex == index)
            break;

        layoutOffset += m_descriptorLayouts[index].GetTotalDescriptorCount();
    }

    return layoutOffset;
}

UINT D3DDescriptorManager::GetSlotOffset(size_t slotIndex, size_t layoutIndex) const noexcept
{
    return GetLayoutOffset(layoutIndex) + m_descriptorLayouts[layoutIndex].GetSlotOffset(slotIndex);
}

UINT D3DDescriptorManager::GetDescriptorOffset(
    size_t slotIndex, size_t layoutIndex, UINT descriptorIndex
) const noexcept {
    return GetSlotOffset(slotIndex, layoutIndex) + descriptorIndex;
}

UINT D3DDescriptorManager::GetRootIndex(size_t slotIndex, size_t layoutIndex) const noexcept
{
    // Each layout will be ordered one after another.
    auto rootIndex = static_cast<UINT>(slotIndex);

    for (size_t index = 0u; index < layoutIndex; ++index)
        rootIndex += m_descriptorLayouts[index].GetTotalDescriptorCount();

    return rootIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DDescriptorManager::GetCPUHandle(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex
) const noexcept {
    return m_resourceHeapCPU.GetCPUHandle(
        GetDescriptorOffset(registerSlot, registerSpace, descriptorIndex)
    );
}

D3D12_GPU_DESCRIPTOR_HANDLE D3DDescriptorManager::GetGPUHandle(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex
) const noexcept {
    return m_resourceHeapGPU.GetGPUHandle(
        GetDescriptorOffset(registerSlot, registerSpace, descriptorIndex)
    );
}
