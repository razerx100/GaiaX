#include <D3DAllocator.hpp>
#include <Exception.hpp>

std::optional<UINT64> D3DAllocator::Allocate(const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo) noexcept
{
	std::optional<size_t> allocationStart = m_allocator.AllocateN(
		static_cast<size_t>(allocInfo.SizeInBytes), static_cast<size_t>(allocInfo.Alignment)
	);

	if (allocationStart)
		return static_cast<UINT64>(allocationStart.value());

	return {};
}

void D3DAllocator::Deallocate(UINT64 startingAddress, UINT64 bufferSize, UINT64 alignment) noexcept
{
	m_allocator.Deallocate(
		static_cast<size_t>(startingAddress), static_cast<size_t>(bufferSize),
		static_cast<size_t>(alignment)
	);
}

// Memory Manager
MemoryManager::MemoryManager(
	IDXGIAdapter3* adapter, ID3D12Device* device, UINT64 initialBudgetGPU, UINT64 initialBudgetCPU
) : m_adapter{ adapter }, m_device{ device }, m_cpuAllocators{}, m_gpuAllocators{},
	m_availableCPUIndices{}, m_availableGPUIndices{}
{
	const UINT64 availableMemory = GetAvailableMemory();

	if (availableMemory < initialBudgetCPU + initialBudgetGPU)
		throw Exception("MemoryException", "Not Enough memory for allocation.");

	m_cpuAllocators.emplace_back(CreateHeap(D3D12_HEAP_TYPE_UPLOAD, initialBudgetCPU), GetID(true));
	m_gpuAllocators.emplace_back(CreateHeap(D3D12_HEAP_TYPE_DEFAULT, initialBudgetCPU), GetID(false));
}

UINT64 MemoryManager::GetAvailableMemory() const noexcept
{
	// Assuming there is a single GPU for now.
	constexpr UINT gpuNodeIndex                      = 0u;
	// Also only checking the GPU memory.
	constexpr DXGI_MEMORY_SEGMENT_GROUP segmentGroup = DXGI_MEMORY_SEGMENT_GROUP_LOCAL;

	DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo
	{
		.Budget       = 0u,
		.CurrentUsage = 0u
	};

	m_adapter->QueryVideoMemoryInfo(gpuNodeIndex, segmentGroup, &videoMemoryInfo);

	return videoMemoryInfo.Budget - videoMemoryInfo.CurrentUsage;
}

std::uint16_t MemoryManager::GetID(bool cpu) noexcept
{
	std::vector<D3DAllocator>& allocators       = cpu ? m_cpuAllocators : m_gpuAllocators;
	std::queue<std::uint16_t>& availableIndices = cpu ? m_availableCPUIndices : m_availableGPUIndices;

	if (std::empty(availableIndices))
		availableIndices.push(static_cast<std::uint16_t>(std::size(allocators)));

	const std::uint16_t ID = availableIndices.front();
	availableIndices.pop();

	return ID;
}

D3DHeap MemoryManager::CreateHeap(D3D12_HEAP_TYPE type, UINT64 size, bool msaa /* = false */) const
{
	return D3DHeap{ m_device, type, size, msaa };
}
