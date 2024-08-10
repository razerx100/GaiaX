#include <LinearAllocator.hpp>
#include <AllocatorBase.hpp>

LinearAllocator::LinearAllocator() noexcept : m_currentOffset{ 0u } {}

size_t LinearAllocator::SubAllocate(
    size_t subAllocationSize, size_t subAllocationCount, size_t alignment
) noexcept {
    size_t alignedOffset = Align(m_currentOffset, alignment);
    size_t alignedSize = Align(subAllocationSize, alignment);

    m_currentOffset = alignedOffset + (alignedSize * subAllocationCount);

    return alignedOffset;
}

size_t LinearAllocator::GetTotalSize() const noexcept {
    return m_currentOffset;
}
