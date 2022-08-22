#include <MemoryContainer.hpp>
#include <cstring>

MemoryContainer::MemoryContainer(
	std::unique_ptr<std::uint8_t> memory, size_t allocationSize
) noexcept : m_memory{ std::move(memory) }, m_allocationSize{ allocationSize } {}

MemoryContainer::MemoryContainer(MemoryContainer&& other) noexcept
	: m_memory{ std::move(other.m_memory) }, m_allocationSize{ other.m_allocationSize } {}

MemoryContainer& MemoryContainer::operator=(MemoryContainer&& other) noexcept {
	m_memory = std::move(other.m_memory);
	m_allocationSize = other.m_allocationSize;

	return *this;
}

void MemoryContainer::CopyData(std::uint8_t* destination) const {
	memcpy(destination, m_memory.get(), m_allocationSize);
}

void MemoryContainer::ReleaseMemory() noexcept {
	m_memory.reset();
}
