#ifndef D3D_RESOURCE_MANAGER_HPP_
#define D3D_RESOURCE_MANAGER_HPP_
#include <D3DResource.hpp>
#include <LinearAllocator.hpp>
#include <mutex>

template<class ResourceView>
class _D3DResourceManager {
public:
	_D3DResourceManager(
		ResourceType type = ResourceType::gpuOnly,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	) noexcept : m_resourceView{ type, flags } {}

	void ReserveHeapSpace(ID3D12Device* device) {
		m_resourceView.SetBufferInfo(m_allocator.GetTotalSize());
		m_resourceView.ReserveHeapSpace(device);
	}

	void UnlockCPUMutex() noexcept {
		m_cpuMemoryMutex.unlock();
	}

	[[nodiscard]]
	size_t ReserveSpaceAndGetOffset(
		size_t subAllocationSize, size_t subAllocationCount = 1u, size_t alignment = 4u
	) noexcept {
		return m_allocator.SubAllocate(subAllocationSize, subAllocationCount, alignment);
	}
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUStartAddress() const noexcept {
		return m_resourceView.GetGPUAddress();
	}
	[[nodiscard]]
	std::uint8_t* GetCPUStartAddressAndLockMutex() {
		m_cpuMemoryMutex.lock();

		return m_resourceView.GetCPUWPointer();
	}

	std::uint8_t* GetCPUStartAddress() const {
		return m_resourceView.GetCPUWPointer();
	}

protected:
	ResourceView m_resourceView;
	LinearAllocator m_allocator;
	std::mutex m_cpuMemoryMutex;
};
#endif