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
