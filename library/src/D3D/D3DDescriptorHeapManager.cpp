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

void D3DReusableDescriptorHeap::CreateSRV(
    ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc,
    UINT descriptorIndex
) {
    m_descriptorHeap.CreateSRV(resource, srvDesc, descriptorIndex);
}

void D3DReusableDescriptorHeap::CreateCBV(
    const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc, UINT descriptorIndex
) {
    m_descriptorHeap.CreateCBV(cbvDesc, descriptorIndex);
}

void D3DReusableDescriptorHeap::CreateUAV(
    ID3D12Resource* resource, ID3D12Resource* counterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT descriptorIndex
) {
    m_descriptorHeap.CreateUAV(resource, counterResource, uavDesc, descriptorIndex);
}

void D3DReusableDescriptorHeap::CreateDSV(
    ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, UINT descriptorIndex
) {
    m_descriptorHeap.CreateDSV(resource, dsvDesc, descriptorIndex);
}

void D3DReusableDescriptorHeap::CreateRTV(
    ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, UINT descriptorIndex
) {
    m_descriptorHeap.CreateRTV(resource, rtvDesc, descriptorIndex);
}

void D3DReusableDescriptorHeap::CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc, UINT descriptorIndex)
{
    m_descriptorHeap.CreateSampler(samplerDesc, descriptorIndex);
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
	for (const auto& viewMap : m_rootDescriptors)
		viewMap.bindViewFunction(commandList, viewMap.rootIndex, viewMap.bufferAddress);

	for (const auto& tableMap : m_descriptorTables)
		tableMap.bindTableFunction(
			commandList, tableMap.rootIndex, descriptorHeap.GetGPUHandle(tableMap.descriptorIndex)
		);
}

D3DDescriptorMap& D3DDescriptorMap::SetRootCBVGfx(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
    return SetRootDescriptor<&ProxyView<&ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView>>(
        rootIndex, bufferAddress
    );
}

D3DDescriptorMap& D3DDescriptorMap::SetRootCBVCom(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
    return SetRootDescriptor<&ProxyView<&ID3D12GraphicsCommandList::SetComputeRootConstantBufferView>>(
        rootIndex, bufferAddress
    );
}

D3DDescriptorMap& D3DDescriptorMap::SetRootUAVGfx(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
    return SetRootDescriptor<&ProxyView<&ID3D12GraphicsCommandList::SetGraphicsRootUnorderedAccessView>>(
        rootIndex, bufferAddress
    );
}

D3DDescriptorMap& D3DDescriptorMap::SetRootUAVCom(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
    return SetRootDescriptor<&ProxyView<&ID3D12GraphicsCommandList::SetComputeRootUnorderedAccessView>>(
        rootIndex, bufferAddress
    );
}

D3DDescriptorMap& D3DDescriptorMap::SetRootSRVGfx(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
    return SetRootDescriptor<&ProxyView<&ID3D12GraphicsCommandList::SetGraphicsRootShaderResourceView>>(
        rootIndex, bufferAddress
    );
}

D3DDescriptorMap& D3DDescriptorMap::SetRootSRVCom(
	UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
) noexcept {
    return SetRootDescriptor<&ProxyView<&ID3D12GraphicsCommandList::SetComputeRootShaderResourceView>>(
        rootIndex, bufferAddress
    );
}

D3DDescriptorMap& D3DDescriptorMap::SetDescTableGfx(UINT rootIndex, UINT descriptorIndex) noexcept
{
    return SetDescriptorTable<&ProxyTable<&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable>>(
        rootIndex, descriptorIndex
    );
}

D3DDescriptorMap& D3DDescriptorMap::SetDescTableCom(UINT rootIndex, UINT descriptorIndex) noexcept
{
    return SetDescriptorTable<&ProxyTable<&ID3D12GraphicsCommandList::SetComputeRootDescriptorTable>>(
        rootIndex, descriptorIndex
    );
}

// D3D Descriptor Manager
D3DDescriptorManager::D3DDescriptorManager(ID3D12Device* device, size_t layoutCount)
    : m_resourceHeapGPU{
        device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
    }, m_descriptorMap{},
    m_resourceHeapCPU{
        device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    }, m_descriptorLayouts{ layoutCount, D3DDescriptorLayout{} }
{}

void D3DDescriptorManager::CreateDescriptors()
{
    UINT totalDescriptorCount = 0u;

    for (const D3DDescriptorLayout& layout : m_descriptorLayouts)
        totalDescriptorCount += layout.GetTotalDescriptorCount();

    // If the total descriptor count is 0, make it one so everything else doesn't fail.
    // As this would only be the case in tests anyway.
    if (!totalDescriptorCount)
        totalDescriptorCount = 1u;

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

void D3DDescriptorManager::CreateCBV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex,
    const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc
) const {
    const UINT descriptorIndexInHeap = GetDescriptorOffsetCBV(
        registerSlot, registerSpace, descriptorIndex
    );

    m_resourceHeapCPU.CreateCBV(cbvDesc, descriptorIndexInHeap);

    m_resourceHeapGPU.CopyDescriptors(
        m_resourceHeapCPU, 1u, descriptorIndexInHeap, descriptorIndexInHeap
    );
}

void D3DDescriptorManager::CreateSRV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex,
    ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc
) const {
    const UINT descriptorIndexInHeap = GetDescriptorOffsetSRV(
        registerSlot, registerSpace, descriptorIndex
    );

    m_resourceHeapCPU.CreateSRV(resource, srvDesc, descriptorIndexInHeap);

    m_resourceHeapGPU.CopyDescriptors(
        m_resourceHeapCPU, 1u, descriptorIndexInHeap, descriptorIndexInHeap
    );
}

void D3DDescriptorManager::CreateUAV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex,
    ID3D12Resource* resource, ID3D12Resource* counterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc
) const {
    const UINT descriptorIndexInHeap = GetDescriptorOffsetUAV(
        registerSlot, registerSpace, descriptorIndex
    );

    m_resourceHeapCPU.CreateUAV(resource, counterResource, uavDesc, descriptorIndexInHeap);

    m_resourceHeapGPU.CopyDescriptors(
        m_resourceHeapCPU, 1u, descriptorIndexInHeap, descriptorIndexInHeap
    );
}

void D3DDescriptorManager::SetRootCBV(
    size_t registerSlot, size_t registerSpace, D3D12_GPU_VIRTUAL_ADDRESS resourceAddress,
    bool graphicsQueue
) {
    if (!graphicsQueue)
        m_descriptorMap.SetRootCBVCom(GetRootIndex(registerSlot, registerSpace), resourceAddress);
    else
        m_descriptorMap.SetRootCBVGfx(GetRootIndex(registerSlot, registerSpace), resourceAddress);
}

void D3DDescriptorManager::SetRootSRV(
    size_t registerSlot, size_t registerSpace, D3D12_GPU_VIRTUAL_ADDRESS resourceAddress,
    bool graphicsQueue
) {
    if (!graphicsQueue)
        m_descriptorMap.SetRootSRVCom(GetRootIndex(registerSlot, registerSpace), resourceAddress);
    else
        m_descriptorMap.SetRootSRVGfx(GetRootIndex(registerSlot, registerSpace), resourceAddress);
}

void D3DDescriptorManager::SetRootUAV(
    size_t registerSlot, size_t registerSpace, D3D12_GPU_VIRTUAL_ADDRESS resourceAddress,
    bool graphicsQueue
) {
    if (!graphicsQueue)
        m_descriptorMap.SetRootUAVCom(GetRootIndex(registerSlot, registerSpace), resourceAddress);
    else
        m_descriptorMap.SetRootUAVGfx(GetRootIndex(registerSlot, registerSpace), resourceAddress);
}

void D3DDescriptorManager::SetDescriptorTableCBV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex, bool graphicsQueue
) {
    if (!graphicsQueue)
        m_descriptorMap.SetDescTableCom(
            GetRootIndex(registerSlot, registerSpace),
            GetDescriptorOffsetCBV(registerSlot, registerSpace, descriptorIndex)
        );
    else
        m_descriptorMap.SetDescTableGfx(
            GetRootIndex(registerSlot, registerSpace),
            GetDescriptorOffsetCBV(registerSlot, registerSpace, descriptorIndex)
        );
}

void D3DDescriptorManager::SetDescriptorTableSRV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex, bool graphicsQueue
) {
    if (!graphicsQueue)
        m_descriptorMap.SetDescTableCom(
            GetRootIndex(registerSlot, registerSpace),
            GetDescriptorOffsetSRV(registerSlot, registerSpace, descriptorIndex)
        );
    else
        m_descriptorMap.SetDescTableGfx(
            GetRootIndex(registerSlot, registerSpace),
            GetDescriptorOffsetSRV(registerSlot, registerSpace, descriptorIndex)
        );
}

void D3DDescriptorManager::SetDescriptorTableUAV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex, bool graphicsQueue
) {
    if (!graphicsQueue)
        m_descriptorMap.SetDescTableCom(
            GetRootIndex(registerSlot, registerSpace),
            GetDescriptorOffsetUAV(registerSlot, registerSpace, descriptorIndex)
        );
    else
        m_descriptorMap.SetDescTableGfx(
            GetRootIndex(registerSlot, registerSpace),
            GetDescriptorOffsetUAV(registerSlot, registerSpace, descriptorIndex)
        );
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

UINT D3DDescriptorManager::GetDescriptorOffsetCBV(size_t registerIndex, size_t layoutIndex) const noexcept
{
    return GetLayoutOffset(layoutIndex)
        + m_descriptorLayouts[layoutIndex].GetDescriptorOffsetCBV(registerIndex);
}

UINT D3DDescriptorManager::GetDescriptorOffsetSRV(size_t registerIndex, size_t layoutIndex) const noexcept
{
    return GetLayoutOffset(layoutIndex)
        + m_descriptorLayouts[layoutIndex].GetDescriptorOffsetSRV(registerIndex);
}

UINT D3DDescriptorManager::GetDescriptorOffsetUAV(size_t registerIndex, size_t layoutIndex) const noexcept
{
    return GetLayoutOffset(layoutIndex)
        + m_descriptorLayouts[layoutIndex].GetDescriptorOffsetUAV(registerIndex);
}

UINT D3DDescriptorManager::GetDescriptorOffsetCBV(
    size_t registerIndex, size_t layoutIndex, UINT descriptorIndex
) const noexcept {
    return GetDescriptorOffsetCBV(registerIndex, layoutIndex) + descriptorIndex;
}

UINT D3DDescriptorManager::GetDescriptorOffsetSRV(
    size_t registerIndex, size_t layoutIndex, UINT descriptorIndex
) const noexcept {
    return GetDescriptorOffsetSRV(registerIndex, layoutIndex) + descriptorIndex;
}

UINT D3DDescriptorManager::GetDescriptorOffsetUAV(
    size_t registerIndex, size_t layoutIndex, UINT descriptorIndex
) const noexcept {
    return GetDescriptorOffsetUAV(registerIndex, layoutIndex) + descriptorIndex;
}

UINT D3DDescriptorManager::GetRootIndex(size_t registerIndex, size_t layoutIndex) const noexcept
{
    // Each layout will be ordered one after another.
    auto rootIndex = static_cast<UINT>(registerIndex);

    for (size_t index = 0u; index < layoutIndex; ++index)
        rootIndex += static_cast<UINT>(m_descriptorLayouts[index].GetBindingCount());

    return rootIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DDescriptorManager::GetCPUHandleCBV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex
) const noexcept {
    return m_resourceHeapCPU.GetCPUHandle(
        GetDescriptorOffsetCBV(registerSlot, registerSpace, descriptorIndex)
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DDescriptorManager::GetCPUHandleSRV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex
) const noexcept {
    return m_resourceHeapCPU.GetCPUHandle(
        GetDescriptorOffsetSRV(registerSlot, registerSpace, descriptorIndex)
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DDescriptorManager::GetCPUHandleUAV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex
) const noexcept {
    return m_resourceHeapCPU.GetCPUHandle(
        GetDescriptorOffsetUAV(registerSlot, registerSpace, descriptorIndex)
    );
}

D3D12_GPU_DESCRIPTOR_HANDLE D3DDescriptorManager::GetGPUHandleCBV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex
) const noexcept {
    return m_resourceHeapGPU.GetGPUHandle(
        GetDescriptorOffsetCBV(registerSlot, registerSpace, descriptorIndex)
    );
}

D3D12_GPU_DESCRIPTOR_HANDLE D3DDescriptorManager::GetGPUHandleSRV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex
) const noexcept {
    return m_resourceHeapGPU.GetGPUHandle(
        GetDescriptorOffsetSRV(registerSlot, registerSpace, descriptorIndex)
    );
}

D3D12_GPU_DESCRIPTOR_HANDLE D3DDescriptorManager::GetGPUHandleUAV(
    size_t registerSlot, size_t registerSpace, UINT descriptorIndex
) const noexcept {
    return m_resourceHeapGPU.GetGPUHandle(
        GetDescriptorOffsetUAV(registerSlot, registerSpace, descriptorIndex)
    );
}
