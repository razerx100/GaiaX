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
	void Release() noexcept;

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

	void SetBufferInfo(UINT64 bufferSize) noexcept;
	void SetTextureInfo(UINT64 width, UINT height, DXGI_FORMAT format, bool msaa) noexcept;
	void ReserveHeapSpace(ID3D12Device* device) noexcept;
	void CreateResource(
		ID3D12Device* device, D3D12_RESOURCE_STATES initialState,
		const D3D12_CLEAR_VALUE* clearValue = nullptr
	);
	void ReleaseResource() noexcept;

	static UINT64 QueryTextureBufferSize(
		ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
	) noexcept;
	static UINT64 CalculateRowPitch(UINT64 width) noexcept;

	[[nodiscard]]
	ID3D12Resource* GetResource() const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUWPointer() const;
	[[nodiscard]]
	ResourceType GetResourceType() const noexcept;
	[[nodiscard]]
	D3D12_RESOURCE_DESC GetResourceDesc() const noexcept;

private:
	static void _setTextureInfo(
		UINT64 width, UINT height, DXGI_FORMAT format, bool msaa,
		D3D12_RESOURCE_DESC& resourceDesc
	) noexcept;
	static void _setBufferInfo(
		UINT64 bufferSize, D3D12_RESOURCE_DESC& resourceDesc
	) noexcept;

private:
	D3DResource m_resource;
	D3D12_RESOURCE_DESC m_resourceDescription;
	size_t m_heapOffset;
	ResourceType m_type;
};

class D3DUploadableResourceView {
public:
	D3DUploadableResourceView(
		ResourceType mainType = ResourceType::gpuOnly,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	) noexcept;

	void SetBufferInfo(UINT64 bufferSize) noexcept;
	void SetTextureInfo(
		ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
	) noexcept;
	void ReserveHeapSpace(ID3D12Device* device) noexcept;
	void CreateResource(
		ID3D12Device* device,
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST
	);
	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

	[[nodiscard]]
	std::uint8_t* GetCPUWPointer() const noexcept;
	[[nodiscard]]
	ID3D12Resource* GetResource() const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const noexcept;

private:
	D3DResourceView m_uploadResource;
	D3DResourceView m_gpuResource;
	bool m_texture;
};
#endif
