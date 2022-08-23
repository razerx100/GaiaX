#ifndef UPLOAD_CONTAINER_HPP_
#define UPLOAD_CONTAINER_HPP_
#include <memory>
#include <vector>

class UploadContainer {
public:
	UploadContainer() noexcept;

	void AddMemory(
		std::unique_ptr<std::uint8_t> memory, size_t size, size_t offset
	) noexcept;
	void AddMemory(
		std::unique_ptr<std::uint8_t> memory, size_t size, std::uint8_t* offset
	) noexcept;
	void SetStartingAddress(std::uint8_t* offset) noexcept;
	void CopyData(std::atomic_size_t& workCount) noexcept;

private:
	struct MemoryData {
		size_t size;
		size_t offset;
	};

	void _addMemory(std::unique_ptr<std::uint8_t> memory, size_t size, size_t offset) noexcept;

private:
	std::vector<std::unique_ptr<std::uint8_t>> m_memories;
	std::vector<MemoryData> m_memoryData;
	std::uint8_t* m_startingAddress;
};
#endif
