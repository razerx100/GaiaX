#include <DepthBuffer.hpp>
#include <Exception.hpp>

DepthBuffer::DepthBuffer(ID3D12Device* device) : m_maxWidth{ 0u }, m_maxHeight{ 0u }
//, m_depthBuffer{ ResourceType::gpuOnly, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL }
{

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = 1u,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    };

    device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDSVHeap));
}

void DepthBuffer::CreateDepthBufferView(
    ID3D12Device* device, std::uint32_t width, std::uint32_t height
) {
    if (width > m_maxWidth || height > m_maxHeight)
        throw Exception("DepthBuffer Error", "Resolution exceeds max supported resolution");

    static D3D12_CLEAR_VALUE depthValue{ DXGI_FORMAT_D32_FLOAT, { 1.0f, 0u } };

    //m_depthBuffer.SetTextureInfo(width, height, DXGI_FORMAT_D32_FLOAT, false);
    //m_depthBuffer.CreateResource(device, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthValue);

    static constexpr D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
        .Format = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE,
        .Texture2D = {0u}
    };

    //device->CreateDepthStencilView(
    //    m_depthBuffer.GetResource(), &dsvDesc, m_pDSVHeap->GetCPUDescriptorHandleForHeapStart()
    //);
}

void DepthBuffer::ClearDSV(
    ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle
) noexcept {
    commandList->ClearDepthStencilView(
        dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0u, 0u, nullptr
    );
}

void DepthBuffer::ReserveHeapSpace(ID3D12Device* device) noexcept {
    //m_depthBuffer.SetTextureInfo(m_maxWidth, m_maxHeight,DXGI_FORMAT_D32_FLOAT, false);
    //m_depthBuffer.ReserveHeapSpace(device);
}

void DepthBuffer::SetMaxResolution(std::uint32_t width, std::uint32_t height) noexcept {
    m_maxWidth = width;
    m_maxHeight = height;
}

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDSVHandle() const noexcept {
    return m_pDSVHeap->GetCPUDescriptorHandleForHeapStart();
}
