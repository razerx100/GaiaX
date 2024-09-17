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
                .descriptorCount = 0u,
                .descriptorTable = true
            }
        );
        m_offsets.resize(newSize + 1u, 0u);
    }

    m_descriptorDetails[registerSlot] = details;

    UINT offset = 0u;

    for (size_t index = 0u; index < std::size(m_descriptorDetails); ++index)
    {
        m_offsets[index] = offset;

        const DescriptorDetails& descDetails = m_descriptorDetails[index];
        // Root descriptors don't need descriptor handles. So, no need to add the descriptor count.
        if (descDetails.descriptorTable)
            offset += descDetails.descriptorCount;
    }

    // The last offset will be used as the total descriptor count.
    m_offsets.back() = offset;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddCBVTable(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .descriptorCount = descriptorCount,
            .descriptorTable = true
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddConstants(
    size_t registerSlot, UINT uintCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    // This layout is for the CBV_SRV_UAV heap. Since sampler needs a separate heap anyway,
    // not gonna hold any sampler data in this. Instead, to make my life easier
    // I will use the Sampler type to save the constant values. Which needs its own
    // register space and stuff in D3D12. And the descriptorCount is the number of
    // uints. So, putting the table variable to false, so the number doesn't get
    // added to the total descriptor count.
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
            .descriptorCount = uintCount,
            .descriptorTable = false
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddSRVTable(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .descriptorCount = descriptorCount,
            .descriptorTable = true
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddUAVTable(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            .descriptorCount = descriptorCount,
            .descriptorTable = true
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddRootCBV(
    size_t registerSlot, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .descriptorCount = 0u,
            .descriptorTable = false
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddRootSRV(
    size_t registerSlot, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .descriptorCount = 0u,
            .descriptorTable = false
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddRootUAV(
    size_t registerSlot, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        registerSlot,
        DescriptorDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            .descriptorCount = 0u,
            .descriptorTable = false
        }
    );

    return *this;
}
