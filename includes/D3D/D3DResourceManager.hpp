#ifndef D3D_RESOURCE_MANAGER_HPP_
#define D3D_RESOURCE_MANAGER_HPP_
#include <D3DResource.hpp>
#include <LinearAllocator.hpp>
#include <optional>

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

	[[nodiscard]]
	size_t ReserveSpaceSuballocatedAndGetOffset(
		size_t subAllocationSize, size_t subAllocationCount, size_t alignment
	) noexcept {
		return m_allocator.SubAllocate(subAllocationSize, subAllocationCount, alignment);
	}
	[[nodiscard]]
	size_t ReserveSpaceAndGetOffset(size_t subAllocationSize) noexcept {
		return m_allocator.SubAllocate(subAllocationSize, 1u, 4u);
	}
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUStartAddress() const noexcept {
		return m_resourceView.GetFirstGPUAddress();
	}
	[[nodiscard]]
	std::uint8_t* GetCPUStartAddress() const {
		return m_resourceView.GetFirstCPUWPointer();
	}

protected:
	ResourceView m_resourceView;
	LinearAllocator m_allocator;
};

class D3DResourceManager : public _D3DResourceManager<D3DResourceView> {
public:
	struct Args {
		std::optional<ResourceType> type;
		std::optional<D3D12_RESOURCE_FLAGS> flags = D3D12_RESOURCE_FLAG_NONE;
	};

public:
	D3DResourceManager(const Args& arguments);

	void CreateResource(ID3D12Device* device);
};

class D3DUploadableResourceManager : public _D3DResourceManager<D3DUploadableResourceView> {
public:
	void CreateResource(ID3D12Device* device);
	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;
};
#endif