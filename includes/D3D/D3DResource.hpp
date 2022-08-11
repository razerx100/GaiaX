#ifndef D3D_RESOURCE_HPP_
#define D3D_RESOURCE_HPP_

#include <D3DHeaders.hpp>
#include <memory>

class D3DResource {
public:
	D3DResource() noexcept;
	virtual ~D3DResource() = default;

	void CreateResource(
		ID3D12Device* device, ID3D12Heap* heap, size_t offset, const D3D12_RESOURCE_DESC& desc,
		D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue = nullptr
	);
	void MapBuffer();

	[[nodiscard]]
	ID3D12Resource* Get() const noexcept;
	[[nodiscard]]
	ID3D12Resource** GetAddress() noexcept;
	[[nodiscard]]
	ID3D12Resource** ReleaseAndGetAddress() noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUWPointer() const noexcept;

private:
	ComPtr<ID3D12Resource> m_pBuffer;
	std::uint8_t* m_cpuHandle;
};

using D3DResourceShared = std::shared_ptr<D3DResource>;

enum class ResourceType {
	upload,
	cpuWrite,
	cpuReadBack,
	gpuOnly
};

class D3DResourceView {
public:
	D3DResourceView(
		ResourceType type, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	) noexcept;

	void SetBufferInfo(UINT64 alignment, UINT64 bufferSize) noexcept;
	void SetTextureInfo(
		UINT64 alignment, UINT64 width, UINT height, DXGI_FORMAT format
	) noexcept;
	void ReserveHeapSpace(ID3D12Device* device) noexcept;
	void CreateResource(
		ID3D12Device* device, D3D12_RESOURCE_STATES initialState,
		const D3D12_CLEAR_VALUE* clearValue = nullptr
	);

	[[nodiscard]]
	ID3D12Resource* GetResource() const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUWPointer() const;
	[[nodiscard]]
	ResourceType GetResourceType() const noexcept;

private:
	D3DResource m_resource;
	D3D12_RESOURCE_DESC m_resourceDescription;
	size_t m_heapOffset;
	ResourceType m_type;
};
#endif
