#ifndef D3D_RESOURCE_BUFFER_HPP_
#define D3D_RESOURCE_BUFFER_HPP_
#include <D3DResources.hpp>
#include <LinearAllocator.hpp>
#include <optional>

template<class ResourceView>
class _D3DResourceBuffer {
public:
	_D3DResourceBuffer(
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

class D3DResourceBuffer : public _D3DResourceBuffer<D3DResourceView> {
public:
	D3DResourceBuffer(ResourceType type, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void CreateResource(ID3D12Device* device);
};

class D3DUploadableResourceBuffer : public _D3DResourceBuffer<D3DUploadableResourceView> {
public:
	void CreateResource(ID3D12Device* device);
	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;
};
#endif