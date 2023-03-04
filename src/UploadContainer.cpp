#include <UploadContainer.hpp>
#include <cstring>
#include <Gaia.hpp>
#include <D3DHelperFunctions.hpp>

UploadContainer::UploadContainer() noexcept : m_startingAddress{ nullptr } {}

void UploadContainer::AddMemory(void* memoryRef, size_t size, size_t offset) noexcept {
	m_memoryRefs.emplace_back(memoryRef);
	m_memoryData.emplace_back(size, offset);
}

void UploadContainer::SetStartingAddress(std::uint8_t* offset) noexcept {
	m_startingAddress = offset;
}

void UploadContainer::CopyData(std::atomic_size_t& workCount) const noexcept {
	++workCount;

	Gaia::threadPool->SubmitWork(
		[&] {
			for (size_t index = 0u; index < std::size(m_memoryRefs); ++index) {
				const MemoryData& memoryData = m_memoryData[index];

				memcpy(
					m_startingAddress + memoryData.offset, m_memoryRefs[index], memoryData.size
				);
			}

			--workCount;
		}
	);
}

// Upload Container Texture
void UploadContainerTexture::AddMemory(
	std::unique_ptr<std::uint8_t> memory, size_t rowPitch, size_t height,
	std::uint8_t* addressStart
) noexcept {
	m_memories.emplace_back(std::move(memory));
	m_memoryData.emplace_back(rowPitch, height, addressStart);
}

void UploadContainerTexture::CopyData(std::atomic_size_t& workCount) const noexcept {
	++workCount;

	Gaia::threadPool->SubmitWork(
		[&] {
			for (size_t index = 0u; index < std::size(m_memories); ++index) {
				const MemoryData& memoryData = m_memoryData[index];
				const size_t alignedRowPitch = Align(
					memoryData.rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
				);
				std::uint8_t const* srcData = m_memories[index].get();

				for (size_t row = 0; row < memoryData.height; ++row)
					memcpy(
						memoryData.startingAddress + (alignedRowPitch * row),
						srcData + (memoryData.rowPitch * row), memoryData.rowPitch
					);
			}

			--workCount;
		}
	);
}
