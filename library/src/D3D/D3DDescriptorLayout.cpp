#include <D3DDescriptorLayout.hpp>
#include <ranges>
#include <algorithm>
#include <cassert>

// D3D Descriptor Layout
std::optional<size_t> D3DDescriptorLayout::FindBindingIndex(
    UINT registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE type
) const noexcept {
    auto result = std::ranges::find_if(
        m_bindingDetails, [registerIndex, type](const BindingDetails& bindingDetails)
        {
            return bindingDetails.registerIndex == registerIndex && bindingDetails.type == type;
        }
    );

    std::optional<size_t> bindingIndex{};

    if (result != std::end(m_bindingDetails))
        bindingIndex = std::distance(std::begin(m_bindingDetails), result);

    return bindingIndex;
}

void D3DDescriptorLayout::AddView(const BindingDetails& details) noexcept
{
    std::optional<size_t> oBindingIndex = FindBindingIndex(details.registerIndex, details.type);
    size_t bindingIndex                 = std::numeric_limits<size_t>::max();

    if (!oBindingIndex)
        m_bindingDetails.emplace_back(details);
    else
    {
        bindingIndex                   = oBindingIndex.value();
        m_bindingDetails[bindingIndex] = details;
    }

    // Update the offsets.
    {
        UINT offset = 0u;

        for (size_t index = 0u; index < std::size(m_bindingDetails); ++index)
        {
            m_offsets[index] = offset;

            const BindingDetails& bindingDetails = m_bindingDetails[index];
            // Root descriptors and constants don't need descriptor handles.
            // So, no need to add the descriptor count.
            if (bindingDetails.descriptorTable)
                offset += bindingDetails.descriptorCount;
        }

        // The last offset will be used as the total descriptor count.
        m_offsets.back() = offset;
    }
}

UINT D3DDescriptorLayout::GetRegisterOffset(
    size_t registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE type
) const noexcept {
    std::optional<size_t> bindingIndex = FindBindingIndex(static_cast<UINT>(registerIndex), type);

    assert(bindingIndex && "Register doesn't have a binding.");

    return m_offsets[bindingIndex.value()];
}

D3DDescriptorLayout& D3DDescriptorLayout::AddCBVTable(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .descriptorCount = descriptorCount,
            .registerIndex   = static_cast<UINT>(registerSlot),
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
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
            .descriptorCount = uintCount,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = false
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddSRVTable(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .descriptorCount = descriptorCount,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = true
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddUAVTable(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            .descriptorCount = descriptorCount,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = true
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddRootCBV(
    size_t registerSlot, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .descriptorCount = 0u,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = false
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddRootSRV(
    size_t registerSlot, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .descriptorCount = 0u,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = false
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddRootUAV(
    size_t registerSlot, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            .descriptorCount = 0u,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = false
        }
    );

    return *this;
}
