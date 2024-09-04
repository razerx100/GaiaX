#include <D3DDescriptorLayout.hpp>

// D3D Descriptor Layout
void D3DDescriptorLayout::AddView(size_t registerSlot, const DescriptorDetails& details) noexcept
{
    if (registerSlot >= std::size(m_descriptorDetails))
        m_descriptorDetails.resize(registerSlot + 1u);

    m_descriptorDetails[registerSlot] = details;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddCBV(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .descriptorCount = descriptorCount
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddSRV(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .descriptorCount = descriptorCount
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddUAV(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            .descriptorCount = descriptorCount
        }
    );

    return *this;
}

UINT D3DDescriptorLayout::GetTotalDescriptorCount() const noexcept
{
    UINT sum = 0u;

    for (const auto& details : m_descriptorDetails)
        sum += details.descriptorCount;

    return sum;
}
