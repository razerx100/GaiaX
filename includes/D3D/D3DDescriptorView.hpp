#ifndef D3D_DESCRIPTOR_VIEW_HPP_
#define D3D_DESCRIPTOR_VIEW_HPP_
#include <AddressContainer.hpp>
#include <GaiaDataTypes.hpp>
#include <D3DResource.hpp>
#include <vector>

class D3DRootDescriptorView {
public:
	void SetAddressesStart(size_t addressesStart, size_t subAllocationSize) noexcept;

	void UpdateCPUAddressStart(std::uint8_t* offset) noexcept;
	void UpdateGPUAddressStart(D3D12_GPU_VIRTUAL_ADDRESS offset) noexcept;

	[[nodiscard]]
	std::uint8_t* GetCPUAddressStart(size_t index) const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddressStart(size_t index) const noexcept;

private:
	SingleAddressContainer m_cpuAddress;
	SingleAddressContainer m_gpuAddress;
};

class D3DDescriptorView {
public:
	D3DDescriptorView(
		ResourceType type, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	);

	void SetDescriptorHandles(
		SharedCPUHandle cpuHandle, SharedGPUHandle gpuHandle, size_t descriptorSize
	) noexcept;
	void SetBufferInfo(
		ID3D12Device* device,
		UINT strideSize, UINT elementsPerAllocation, size_t subAllocationCount
	) noexcept;

	void CreateDescriptorView(ID3D12Device* device);

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(size_t index) const noexcept;
	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(size_t index) const noexcept;

	[[nodiscard]]
	UINT64 GetCounterOffset(size_t index) const noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUWPointer(size_t index) const noexcept;

private:
	D3DResourceView m_resourceBuffer;
	SharedCPUHandle m_sharedCPUHandle;
	SharedGPUHandle m_sharedGPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandleStart;
	size_t m_descriptorSize;
	bool m_isUAV;
	size_t m_subAllocationSize;
	size_t m_strideSize;
	std::vector<D3D12_BUFFER_UAV> m_bufferInfos;
};
#endif
