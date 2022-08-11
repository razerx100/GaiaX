#include <LinearAllocator.hpp>
#include <D3DHelperFunctions.hpp>
#include <D3DThrowMacros.hpp>

LinearAllocator::LinearAllocator() noexcept : m_currentOffset{ 0u } {}

size_t LinearAllocator::SubAllocate(size_t size, size_t alignment) noexcept {
    size_t alignedOffset = Align(m_currentOffset, alignment);

    m_currentOffset = alignedOffset + size;

    return alignedOffset;
}

size_t LinearAllocator::GetTotalSize() const noexcept {
    return m_currentOffset;
}
