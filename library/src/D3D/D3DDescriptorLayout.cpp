#include <D3DDescriptorLayout.hpp>
#include <ranges>
#include <algorithm>

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
    {
        m_bindingDetails.emplace_back(details);
        m_offsets.emplace_back(0u);
    }
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
            const BindingDetails& bindingDetails = m_bindingDetails[index];
            // Root descriptors and constants don't need descriptor handles.
            // So, no need to add the descriptor count.
            if (bindingDetails.descriptorTable)
            {
                m_offsets[index] = offset;
                offset          += bindingDetails.descriptorCount;
            }
            else
                // Set the offset to zero if it is not a table.
                // This is necessary as I am using the last offset as
                // the total descriptor count.
                m_offsets[index] = 0u;
        }

        // The last offset will be used as the total descriptor count.
        m_offsets.back() = offset;
    }
}

D3DDescriptorLayout& D3DDescriptorLayout::AddCBVTable(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage,
    bool bindless /* = false */
) noexcept {
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .descriptorCount = descriptorCount,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = true,
            .bindless        = bindless
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddConstants(
    size_t registerSlot, UINT uintCount, D3D12_SHADER_VISIBILITY shaderStage
) noexcept {
    // A CBV descriptor which has its table set to false but has a non-zero descriptor count
    // would be constant values.
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .descriptorCount = uintCount,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = false,
            .bindless        = false
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddSRVTable(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage,
    bool bindless /* = false */
) noexcept {
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .descriptorCount = descriptorCount,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = true,
            .bindless        = bindless
        }
    );

    return *this;
}

D3DDescriptorLayout& D3DDescriptorLayout::AddUAVTable(
    size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage,
    bool bindless /* = false */
) noexcept {
    AddView(
        BindingDetails{
            .visibility      = shaderStage,
            .type            = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            .descriptorCount = descriptorCount,
            .registerIndex   = static_cast<UINT>(registerSlot),
            .descriptorTable = true,
            .bindless        = bindless
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
            .descriptorTable = false,
            .bindless        = false
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
            .descriptorTable = false,
            .bindless        = false
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
            .descriptorTable = false,
            .bindless        = false
        }
    );

    return *this;
}
