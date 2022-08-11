#ifndef LINEAR_ALLOCATOR_HPP_
#define LINEAR_ALLOCATOR_HPP_

class LinearAllocator {
public:
    LinearAllocator() noexcept;

    [[nodiscard]]
    size_t SubAllocate(size_t size, size_t alignment) noexcept;

    [[nodiscard]]
    size_t GetTotalSize() const noexcept;

private:
    size_t m_currentOffset;
};
#endif
