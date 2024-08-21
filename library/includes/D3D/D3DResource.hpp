#ifndef D3D_RESOURCE_HPP_
#define D3D_RESOURCE_HPP_
#include <cstdint>
#include <utility>
#include <D3DHeaders.hpp>
#include <D3DAllocator.hpp>

class Resource
{
public:
	Resource(MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType);
	~Resource() noexcept;

	[[nodiscard]]
	UINT64 AllocationSize() const noexcept { return m_allocationInfo.size; }

protected:
	[[nodiscard]]
	UINT64 GetHeapOffset() const noexcept { return m_allocationInfo.heapOffset; }
	[[nodiscard]]
	ID3D12Heap* GetHeap() const noexcept { return m_allocationInfo.heap; }
	[[nodiscard]]
	D3D12_HEAP_TYPE GetHeapType() const noexcept { return m_resourceType; }

	void Allocate(const D3D12_RESOURCE_DESC& resourceDesc);
	void Deallocate() noexcept;

private:
	void SelfDestruct() noexcept;

private:
	MemoryManager*                  m_memoryManager;
	MemoryManager::MemoryAllocation m_allocationInfo;
	D3D12_HEAP_TYPE                 m_resourceType;

public:
	Resource(const Resource&) = delete;
	Resource& operator=(const Resource&) = delete;

	Resource(Resource&& other) noexcept
		: m_memoryManager{ std::exchange(other.m_memoryManager, nullptr) },
		m_allocationInfo{ other.m_allocationInfo }, m_resourceType{ other.m_resourceType }
	{
		// Setting the allocation validity to false, so the other object doesn't deallocate.
		// Don't need to deallocate our previous data, as there was none.
		other.m_allocationInfo.isValid = false;
	}
	Resource& operator=(Resource&& other) noexcept
	{
		// Deallocating the already existing memory.
		SelfDestruct();

		m_memoryManager  = std::exchange(other.m_memoryManager, nullptr);
		m_allocationInfo = other.m_allocationInfo;
		m_resourceType   = other.m_resourceType;
		// Taking the allocation data from the other object.
		other.m_allocationInfo.isValid = false;
		// So, setting the allocation validity to false, so the other object doesn't deallocate.

		return *this;
	}
};

class Buffer : public Resource
{
public:
	Buffer(ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType);

	void Create(UINT64 bufferSize, D3D12_RESOURCE_STATES initialState);
	void Destroy() noexcept;

	[[nodiscard]]
	ID3D12Resource* Get() const noexcept { return m_buffer.Get(); }
	[[nodiscard]]
	std::uint8_t* CPUHandle() const noexcept { return m_cpuHandle; }
	[[nodiscard]]
	UINT64 BufferSize() const noexcept { return m_bufferSize; }

private:
	ComPtr<ID3D12Resource> m_buffer;
	std::uint8_t*          m_cpuHandle;
	ID3D12Device*          m_device;
	UINT64                 m_bufferSize;

public:
	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	Buffer(Buffer&& other) noexcept
		: Resource{ std::move(other) }, m_buffer{ std::move(other.m_buffer) },
		m_cpuHandle{ std::exchange(other.m_cpuHandle, nullptr) }, m_device{ other.m_device },
		m_bufferSize{ other.m_bufferSize }
	{}
	Buffer& operator=(Buffer&& other) noexcept
	{
		Resource::operator=(std::move(other));

		m_buffer     = std::move(other.m_buffer);
		m_cpuHandle  = std::exchange(other.m_cpuHandle, nullptr);
		m_device     = other.m_device;
		m_bufferSize = other.m_bufferSize;

		return *this;
	}
};

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

	void SetBufferInfo(
		UINT64 bufferSize, UINT64 allocationCount = 1u, UINT64 alignment = 4u
	) noexcept;
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
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(UINT64 index) const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetFirstGPUAddress() const noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUWPointer(UINT64 index) const noexcept;
	[[nodiscard]]
	std::uint8_t* GetFirstCPUWPointer() const noexcept;
	[[nodiscard]]
	ResourceType GetResourceType() const noexcept;
	[[nodiscard]]
	D3D12_RESOURCE_DESC GetResourceDesc() const noexcept;
	[[nodiscard]]
	UINT64 GetSubAllocationOffset(UINT64 index) const noexcept;
	[[nodiscard]]
	UINT64 GetFirstSubAllocationOffset() const noexcept;

private:
	static void _setTextureInfo(
		UINT64 width, UINT height, DXGI_FORMAT format, bool msaa,
		D3D12_RESOURCE_DESC& resourceDesc
	) noexcept;
	static void _setBufferInfo(UINT64 bufferSize, D3D12_RESOURCE_DESC& resourceDesc) noexcept;

private:
	D3DResource m_resource;
	D3D12_RESOURCE_DESC m_resourceDescription;
	size_t m_heapOffset;
	ResourceType m_type;
	UINT64 m_subAllocationSize;
	UINT64 m_subAllocationCount;
	UINT64 m_subBufferSize;
};

class D3DUploadableResourceView {
public:
	D3DUploadableResourceView(
		ResourceType mainType = ResourceType::gpuOnly,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	) noexcept;

	void SetBufferInfo(
		UINT64 bufferSize, UINT64 allocationCount = 1u, UINT64 alignment = 4u
	) noexcept;
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
	ID3D12Resource* GetResource() const noexcept;
	[[nodiscard]]
	D3D12_RESOURCE_DESC GetResourceDesc() const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(UINT64 index) const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetFirstGPUAddress() const noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUWPointer(UINT64 index) const noexcept;
	[[nodiscard]]
	std::uint8_t* GetFirstCPUWPointer() const noexcept;
	[[nodiscard]]
	UINT64 GetSubAllocationOffset(UINT64 index) const noexcept;
	[[nodiscard]]
	UINT64 GetFirstSubAllocationOffset() const noexcept;

private:
	D3DResourceView m_uploadResource;
	D3DResourceView m_gpuResource;
	bool m_texture;
};

// Helper Functions
template<typename T>
void SetResourceViewBufferInfo(
	ID3D12Device* device, UINT64 bufferSize, T& resView, UINT64 allocationCount = 1u,
	UINT64 alignment = 4u
) noexcept {
	resView.SetBufferInfo(bufferSize, allocationCount, alignment);
	resView.ReserveHeapSpace(device);
}
#endif
