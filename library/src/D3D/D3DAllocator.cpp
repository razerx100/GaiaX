#include <D3DAllocator.hpp>

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
