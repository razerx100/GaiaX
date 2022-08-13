#ifndef D3D_SINGLE_RESOURCE_MANAGER_HPP_
#define D3D_SINGLE_RESOURCE_MANAGER_HPP_
#include <D3DResource.hpp>
#include <LinearAllocator.hpp>

class D3DSingleResourceManager {
public:
	D3DSingleResourceManager(
		ResourceType type, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	);

	void CreateResource(ID3D12Device* device);
	void ReserveHeapSpace(ID3D12Device* device);

	[[nodiscard]]
	size_t ReserveSpaceAndGetOffset(
		size_t subAllocationSize, size_t subAllocationCount, size_t alignment
	) noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUStartAddress() const noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUStartAddress() const;

private:
	D3DResourceView m_resourceView;
	LinearAllocator m_linearAllocator;
};
#endif
