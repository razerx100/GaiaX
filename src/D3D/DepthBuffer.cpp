#include <DepthBuffer.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>

DepthBuffer::DepthBuffer(
    ID3D12Device* device
) {
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr;
    D3D_THROW_FAILED(hr, device->CreateDescriptorHeap(
        &dsvHeapDesc, __uuidof(ID3D12DescriptorHeap), &m_pDSVHeap
    ));
}

void DepthBuffer::CreateDepthBuffer(
    ID3D12Device* device,
	std::uint32_t width, std::uint32_t height
) {
    D3D12_CLEAR_VALUE depthValue = {};
    depthValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthValue.DepthStencil = { 1.0f, 0 };

    CD3DX12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    CD3DX12_RESOURCE_DESC rsDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT, width, height,
        1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );

    HRESULT hr;
    D3D_THROW_FAILED(hr, device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthValue,
        __uuidof(ID3D12Resource),
        &m_pDepthBuffer
    ));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(
        m_pDepthBuffer.Get(), &dsvDesc,
        m_pDSVHeap->GetCPUDescriptorHandleForHeapStart()
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDSVHandle() const noexcept {
    return m_pDSVHeap->GetCPUDescriptorHandleForHeapStart();
}

void DepthBuffer::ClearDSV(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) noexcept {
    commandList->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr
    );
}
