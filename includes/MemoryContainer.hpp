#ifndef MEMORY_CONTAINER_HPP_
#define MEMORY_CONTAINER_HPP_
#include <memory>

class MemoryContainer {
public:
	MemoryContainer() = delete;
	MemoryContainer(const MemoryContainer&) = delete;

	MemoryContainer(MemoryContainer&& other) noexcept;
	MemoryContainer(std::unique_ptr<std::uint8_t> memory, size_t allocationSize) noexcept;

	MemoryContainer& operator=(const MemoryContainer&) = delete;

	MemoryContainer& operator=(MemoryContainer&& other) noexcept;

	void CopyData(std::uint8_t* destination) const;
	void ReleaseMemory() noexcept;

private:
	std::unique_ptr<std::uint8_t> m_memory;
	size_t m_allocationSize;
};
#endif
