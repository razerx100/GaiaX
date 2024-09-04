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
	};

public:
	D3DDescriptorLayout() : m_descriptorDetails{} {}

	D3DDescriptorLayout& AddCBV(
		size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept;
	D3DDescriptorLayout& AddSRV(
		size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept;
	D3DDescriptorLayout& AddUAV(
		size_t registerSlot, UINT descriptorCount, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept;

	[[nodiscard]]
	const std::vector<DescriptorDetails>& GetDetails() const noexcept { return m_descriptorDetails; }

	[[nodiscard]]
	UINT GetTotalDescriptorCount() const noexcept;

private:
	void AddView(size_t registerSlot, const DescriptorDetails& details) noexcept;

private:
	std::vector<DescriptorDetails> m_descriptorDetails;
};
#endif
