#ifndef D3D_DESCRIPTOR_LAYOUT_HPP_
#define D3D_DESCRIPTOR_LAYOUT_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <array>
#include <optional>

class D3DDescriptorLayout
{
public:
	struct BindingDetails
	{
		D3D12_SHADER_VISIBILITY     visibility;
		D3D12_DESCRIPTOR_RANGE_TYPE type;
		UINT                        descriptorCount;
		UINT                        registerIndex;
		bool                        descriptorTable;
	};

public:
	D3DDescriptorLayout() : m_bindingDetails{}, m_offsets{ 0u } {}

	D3DDescriptorLayout& AddConstants(
		size_t registerSlot, UINT uintCount, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept;
	D3DDescriptorLayout& AddCBVTable(
		size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept;
	D3DDescriptorLayout& AddSRVTable(
		size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept;
	D3DDescriptorLayout& AddUAVTable(
		size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept;
	D3DDescriptorLayout& AddRootCBV(size_t registerSlot, D3D12_SHADER_VISIBILITY shaderStage) noexcept;
	D3DDescriptorLayout& AddRootSRV(size_t registerSlot, D3D12_SHADER_VISIBILITY shaderStage) noexcept;
	D3DDescriptorLayout& AddRootUAV(size_t registerSlot, D3D12_SHADER_VISIBILITY shaderStage) noexcept;

	[[nodiscard]]
	BindingDetails GetBindingDetails(size_t bindingIndex) const noexcept
	{
		return m_bindingDetails[bindingIndex];
	}
	[[nodiscard]]
	size_t GetBindingCount() const noexcept { return std::size(m_bindingDetails); }
	[[nodiscard]]
	UINT GetRegisterOffset(size_t registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE type) const noexcept;
	[[nodiscard]]
	UINT GetTotalDescriptorCount() const noexcept { return m_offsets.back(); }

private:
	void AddView(const BindingDetails& details) noexcept;

	[[nodiscard]]
	std::optional<size_t> FindBindingIndex(
		UINT registerIndex, D3D12_DESCRIPTOR_RANGE_TYPE type
	) const noexcept;

private:
	std::vector<BindingDetails> m_bindingDetails;
	std::vector<UINT>           m_offsets;

public:
	D3DDescriptorLayout(const D3DDescriptorLayout& other) noexcept
		: m_bindingDetails{ other.m_bindingDetails },
		m_offsets{ other.m_offsets }
	{}
	D3DDescriptorLayout& operator=(const D3DDescriptorLayout& other) noexcept
	{
		m_bindingDetails = other.m_bindingDetails;
		m_offsets        = other.m_offsets;

		return *this;
	}
	D3DDescriptorLayout(D3DDescriptorLayout&& other) noexcept
		: m_bindingDetails{ std::move(other.m_bindingDetails) },
		m_offsets{ std::move(other.m_offsets) }
	{}
	D3DDescriptorLayout& operator=(D3DDescriptorLayout&& other) noexcept
	{
		m_bindingDetails = std::move(other.m_bindingDetails);
		m_offsets        = std::move(other.m_offsets);

		return *this;
	}
};
#endif
