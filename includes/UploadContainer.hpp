#ifndef UPLOAD_CONTAINER_HPP_
#define UPLOAD_CONTAINER_HPP_
#include <memory>
#include <vector>
#include <atomic>

class UploadContainer {
public:
	UploadContainer() noexcept;

	void AddMemory(void* memoryRef, size_t size, size_t offset) noexcept;
	void AddMemory(
		std::unique_ptr<std::uint8_t> memory, size_t rowPitch, size_t height,
		std::uint8_t* addressStart
	) noexcept;

	void SetStartingAddress(std::uint8_t* offset) noexcept;
	void CopyData(std::atomic_size_t& workCount) const noexcept;

private:
	struct MemoryData {
		size_t size;
		size_t offset;
	};

	struct TextureData {
		size_t rowPitch;
		size_t height;
		std::uint8_t* startingAddress;
	};

private:
	std::vector<void*> m_memoryRefs;
	std::vector<MemoryData> m_memoryData;
	std::uint8_t* m_startingAddress;
	std::vector<std::unique_ptr<std::uint8_t>> m_textureMemories;
	std::vector<TextureData> m_textureData;
};
#endif
