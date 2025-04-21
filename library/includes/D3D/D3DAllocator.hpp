#ifndef D3D_ALLOCATOR_HPP_
#define D3D_ALLOCATOR_HPP_
#include <Buddy.hpp>
#include <D3DHeap.hpp>
#include <optional>
#include <queue>

class D3DAllocator
{
public:
	D3DAllocator(D3DHeap&& heap, std::uint16_t id)
		: m_heap{ std::move(heap) },
		m_allocator{ 0u, static_cast<size_t>(m_heap.Size()), 256_B },
		m_id{ id }
	{}

	[[nodiscard]]
	// Returns false if there is not enough memory.
	std::optional<UINT64> Allocate(const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo) noexcept;

	void Deallocate(UINT64 startingAddress, UINT64 bufferSize, UINT64 alignment) noexcept;

	[[nodiscard]]
	std::uint16_t GetID() const noexcept { return m_id; }
	[[nodiscard]]
	UINT64 Size() const noexcept { return m_heap.Size(); }
	[[nodiscard]]
	UINT64 AvailableSize() const noexcept
	{
		return static_cast<UINT64>(m_allocator.AvailableSize());
	}
	[[nodiscard]]
	ID3D12Heap* GetHeap() const noexcept { return m_heap.Get(); }

private:
	D3DHeap         m_heap;
	Callisto::Buddy m_allocator;
	std::uint16_t   m_id;

public:
	D3DAllocator(const D3DAllocator&) = delete;
	D3DAllocator& operator=(const D3DAllocator&) = delete;

	D3DAllocator(D3DAllocator&& other) noexcept
		: m_heap{ std::move(other.m_heap) }, m_allocator{ std::move(other.m_allocator) },
		m_id{ other.m_id }
	{}
	D3DAllocator& operator=(D3DAllocator&& other) noexcept
	{
		m_heap      = std::move(other.m_heap);
		m_allocator = std::move(other.m_allocator);
		m_id        = other.m_id;

		return *this;
	}
};

class MemoryManager
{
public:
	struct MemoryAllocation
	{
		UINT64        heapOffset;
		ID3D12Heap*   heap;
		UINT64        size;
		UINT64        alignment;
		std::uint16_t memoryID;
		bool          isValid = false;
	};

public:
	MemoryManager(
		IDXGIAdapter3* adapter, ID3D12Device* device, UINT64 initialBudgetGPU,
		UINT64 initialBudgetCPU
	);

	[[nodiscard]]
	MemoryAllocation Allocate(
		const D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE heapType, bool msaa = false
	);

	void Deallocate(
		const MemoryAllocation& allocation, D3D12_HEAP_TYPE heapType, bool msaa = false
	) noexcept;

private:
	[[nodiscard]]
	std::vector<D3DAllocator>& GetAllocators(bool cpu, bool msaa = false) noexcept;
	[[nodiscard]]
	std::queue<std::uint16_t>& GetAvailableIndices(bool cpu, bool msaa = false) noexcept;
	[[nodiscard]]
	UINT64 GetAvailableMemory() const noexcept;
	[[nodiscard]]
	std::uint16_t GetID(bool cpu, bool msaa = false) noexcept;
	[[nodiscard]]
	D3DHeap CreateHeap(D3D12_HEAP_TYPE type, UINT64 size, bool msaa = false) const;

	[[nodiscard]]
	UINT64 GetNewAllocationSize(D3D12_HEAP_TYPE heapType) const noexcept;

	[[nodiscard]]
	D3D12_RESOURCE_ALLOCATION_INFO GetAllocationInfo(
		const D3D12_RESOURCE_DESC& resourceDesc
	) const noexcept;

private:
	IDXGIAdapter3*            m_adapter;
	ID3D12Device*             m_device;
	std::vector<D3DAllocator> m_cpuAllocators;
	std::vector<D3DAllocator> m_gpuAllocators;
	std::vector<D3DAllocator> m_msaaAllocators;
	std::queue<std::uint16_t> m_availableGPUIndices;
	std::queue<std::uint16_t> m_availableCPUIndices;
	std::queue<std::uint16_t> m_availableMsaaIndices;

public:
	MemoryManager(const MemoryManager&) = delete;
	MemoryManager& operator=(const MemoryManager&) = delete;

	MemoryManager(MemoryManager&& other) noexcept
		: m_adapter{ other.m_adapter }, m_device{ other.m_device },
		m_cpuAllocators{ std::move(other.m_cpuAllocators) },
		m_gpuAllocators{ std::move(other.m_gpuAllocators) },
		m_msaaAllocators{ std::move(other.m_msaaAllocators) },
		m_availableGPUIndices{ std::move(other.m_availableGPUIndices) },
		m_availableCPUIndices{ std::move(other.m_availableCPUIndices) },
		m_availableMsaaIndices{ std::move(other.m_availableMsaaIndices) }
	{}
	MemoryManager& operator=(MemoryManager&& other) noexcept
	{
		m_adapter              = other.m_adapter;
		m_device               = other.m_device;
		m_cpuAllocators        = std::move(other.m_cpuAllocators);
		m_gpuAllocators        = std::move(other.m_gpuAllocators);
		m_msaaAllocators       = std::move(other.m_msaaAllocators);
		m_availableGPUIndices  = std::move(other.m_availableGPUIndices);
		m_availableCPUIndices  = std::move(other.m_availableCPUIndices);
		m_availableMsaaIndices = std::move(other.m_availableMsaaIndices);

		return *this;
	}
};
#endif
