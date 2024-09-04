#include <D3DDescriptorLayout.hpp>

// D3D Descriptor Layout
void D3DDescriptorLayout::AddView(size_t registerSlot, const DescriptorDetails& details) noexcept
{
    if (registerSlot >= std::size(m_descriptorDetails))
    {
        const size_t newSize = registerSlot + 1u;

        m_descriptorDetails.resize(
            newSize, DescriptorDetails{
                .visibility      = D3D12_SHADER_VISIBILITY_ALL,
                .type            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                .descriptorCount = 0u
            }
        );
        m_offsets.resize(newSize + 1u, 0u);
    }

    m_descriptorDetails[registerSlot] = details;

    UINT offset = 0u;

    for (size_t index = 0u; index < std::size(m_descriptorDetails); ++index)
    {
        m_offsets[index] = offset;
        offset          += m_descriptorDetails[index].descriptorCount;
    }

    // The last offset will be used as the total descriptor count.
    m_offsets.back() = offset;
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
