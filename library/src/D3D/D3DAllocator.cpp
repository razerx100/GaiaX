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

D3D12_RESOURCE_ALLOCATION_INFO MemoryManager::GetAllocationInfo(
	const D3D12_RESOURCE_DESC& resourceDesc
) const noexcept {
	return m_device->GetResourceAllocationInfo(0u, 1u, &resourceDesc);
}

UINT64 MemoryManager::GetNewAllocationSize(D3D12_HEAP_TYPE heapType) const noexcept
{
	// Might add some algorithm here later.
	return heapType == D3D12_HEAP_TYPE_DEFAULT ? 2_GB : 100_MB;
}

MemoryManager::MemoryAllocation MemoryManager::Allocate(
	const D3D12_RESOURCE_DESC& resourceDesc, D3D12_HEAP_TYPE heapType
) {
	const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = GetAllocationInfo(resourceDesc);
	const UINT64 bufferSize                             = allocationInfo.SizeInBytes;
	const bool isCPUAccessible                          = heapType == D3D12_HEAP_TYPE_UPLOAD;

	std::vector<D3DAllocator>& allocators = isCPUAccessible ? m_cpuAllocators : m_gpuAllocators;

	// Look through the already existing allocators and try to allocate the buffer.
	// An allocator may still fail even if its total available size is more than the bufferSize.
	// As in a buddy allocator, there must be a block with more than or equal to the size of the
	// allocation.
	for (size_t index = 0u; index < std::size(allocators); ++index)
	{
		D3DAllocator& allocator = allocators[index];

		if (allocator.AvailableSize() >= bufferSize)
		{
			std::optional<UINT64> startingAddress = allocator.Allocate(allocationInfo);

			if (startingAddress)
			{
				const UINT64 offset = startingAddress.value();

				MemoryAllocation allocation
				{
					.heapOffset = offset,
					.heap       = allocator.GetHeap(),
					.size       = bufferSize,
					.alignment  = allocationInfo.Alignment,
					.memoryID   = allocator.GetID(),
					.isValid    = true
				};

				return allocation;
			}
		}
	}

	{
		// If the already available allocators were unable to allocate, then try to allocate new memory.
		UINT64 newAllocationSize         = std::max(bufferSize, GetNewAllocationSize(heapType));

		const UINT64 availableMemorySize = GetAvailableMemory();

		// If allocation is not possible, check if the buffer can be allocated on the available memory.
		if (newAllocationSize > availableMemorySize)
		{
			if (availableMemorySize >= bufferSize)
				newAllocationSize = availableMemorySize;
			else
				throw Exception("MemoryException", "Not Enough memory for allocation.");
		}

		// Gonna add a different vector of msaa allocators later. Then I would have to change
		// here.

		// Since this is a new allocator. If the code reaches here, at least the top most
		// block should have enough memory for allocation.
		D3DAllocator allocator{ CreateHeap(heapType, newAllocationSize), GetID(isCPUAccessible) };

		std::optional<UINT64> startingAddress = allocator.Allocate(allocationInfo);

		if (startingAddress)
		{
			const UINT64 offset = startingAddress.value();

			MemoryAllocation allocation
			{
				.heapOffset = offset,
				.heap       = allocator.GetHeap(),
				.size       = bufferSize,
				.alignment  = allocationInfo.Alignment,
				.memoryID   = allocator.GetID(),
				.isValid    = true
			};

			allocators.emplace_back(std::move(allocator));

			return allocation;
		}
		else
			throw Exception("MemoryException", "Not Enough memory for allocation.");
	}
}

void MemoryManager::Deallocate(const MemoryAllocation& allocation, D3D12_HEAP_TYPE heapType) noexcept
{
	const bool isCPUAccessible            = heapType == D3D12_HEAP_TYPE_UPLOAD;
	std::vector<D3DAllocator>& allocators = isCPUAccessible ? m_cpuAllocators : m_gpuAllocators;

	auto result = std::ranges::find_if(
		allocators,
		[id = allocation.memoryID](const D3DAllocator& alloc) { return alloc.GetID() == id; }
	);

	if (result != std::end(allocators))
	{
		D3DAllocator& allocator = *result;
		allocator.Deallocate(allocation.heapOffset, allocation.size, allocation.alignment);

		// Check if the allocator is fully empty and isn't the last allocator.
		// If so deallocate the empty allocator. Only deallocate CPU accessible allocators
		// if they are empty.
		const bool eraseCondition =
			isCPUAccessible
			&& std::size(allocators) > 1u
			&& allocator.Size() == allocator.AvailableSize();

		if (eraseCondition)
		{
			std::queue<std::uint16_t>& availableIndices
				= isCPUAccessible ? m_availableCPUIndices : m_availableGPUIndices;

			availableIndices.push(allocator.GetID());
			allocators.erase(result);
		}
	}
}
