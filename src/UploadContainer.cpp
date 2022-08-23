#include <UploadContainer.hpp>
#include <cstring>
#include <Gaia.hpp>

UploadContainer::UploadContainer() noexcept : m_startingAddress{ nullptr } {}

void UploadContainer::AddMemory(
	std::unique_ptr<std::uint8_t> memory, size_t size, size_t offset
) noexcept {
	_addMemory(std::move(memory), size, offset);
}

void UploadContainer::AddMemory(
	std::unique_ptr<std::uint8_t> memory, size_t size, std::uint8_t* offset
) noexcept {
	_addMemory(std::move(memory), size, reinterpret_cast<size_t>(offset));
}

void UploadContainer::_addMemory(
	std::unique_ptr<std::uint8_t> memory, size_t size, size_t offset
) noexcept {
	m_memories.emplace_back(std::move(memory));
	m_memoryData.emplace_back(size, offset);
}

void UploadContainer::SetStartingAddress(std::uint8_t* offset) noexcept {
	m_startingAddress = offset;
}

void UploadContainer::CopyData(std::atomic_size_t& workCount) noexcept {
	++workCount;

	Gaia::threadPool->SubmitWork(
		[&] {
			for (size_t index = 0u; index < std::size(m_memories); ++index) {
				const MemoryData& memoryData = m_memoryData[index];

				memcpy(
					m_startingAddress + memoryData.offset,
					m_memories[index].get(), memoryData.size
				);
			}

			--workCount;
		}
	);
}
