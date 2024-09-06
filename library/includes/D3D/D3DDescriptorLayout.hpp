#ifndef D3D_DESCRIPTOR_LAYOUT_HPP_
#define D3D_DESCRIPTOR_LAYOUT_HPP_
#include <D3DHeaders.hpp>
#include <vector>

class D3DDescriptorLayout
{
public:
	struct DescriptorDetails
	{
		D3D12_SHADER_VISIBILITY     visibility;
		D3D12_DESCRIPTOR_RANGE_TYPE type;
		UINT                        descriptorCount;
		bool                        descriptorTable;
	};

public:
	D3DDescriptorLayout() : m_descriptorDetails{}, m_offsets{ 0u } {}

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
	DescriptorDetails GetDescriptorDetails(size_t registerSlot) const noexcept
	{
		return m_descriptorDetails[registerSlot];
	}
	[[nodiscard]]
	size_t GetDescriptorDetailsCount() const noexcept { return std::size(m_descriptorDetails); }
	[[nodiscard]]
	UINT GetSlotOffset(size_t registerSlot) const noexcept { return m_offsets[registerSlot]; }
	[[nodiscard]]
	UINT GetTotalDescriptorCount() const noexcept { return m_offsets.back(); }

private:
	void AddView(size_t registerSlot, const DescriptorDetails& details) noexcept;

private:
	std::vector<DescriptorDetails> m_descriptorDetails;
	std::vector<UINT>              m_offsets;

public:
	D3DDescriptorLayout(const D3DDescriptorLayout& other) noexcept
		: m_descriptorDetails{ other.m_descriptorDetails }, m_offsets{ other.m_offsets }
	{}
	D3DDescriptorLayout& operator=(const D3DDescriptorLayout& other) noexcept
	{
		m_descriptorDetails = other.m_descriptorDetails ;
		m_offsets           = other.m_offsets;

		return *this;
	}
	D3DDescriptorLayout(D3DDescriptorLayout&& other) noexcept
		: m_descriptorDetails{ std::move(other.m_descriptorDetails) },
		m_offsets{ std::move(other.m_offsets) }
	{}
	D3DDescriptorLayout& operator=(D3DDescriptorLayout&& other) noexcept
	{
		m_descriptorDetails = std::move(other.m_descriptorDetails) ;
		m_offsets           = std::move(other.m_offsets);

		return *this;
	}
};
#endif
