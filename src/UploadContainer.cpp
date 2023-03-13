#include <UploadContainer.hpp>
#include <cstring>
#include <Gaia.hpp>
#include <D3DHelperFunctions.hpp>

void UploadContainer::AddMemory(
	void const* srcMemoryRef, void* dstMemoryRef, size_t size
) noexcept {
	MemoryData memData{
		.rowPitch = size,
		.height = 1u,
		.src = srcMemoryRef,
		.dst = dstMemoryRef,
		.texture = false
	};

	m_memoryData.emplace_back(memData);
}

void UploadContainer::AddMemory(
	void const* srcMemoryRef, void* dstMemoryRef, size_t rowPitch, size_t height
) noexcept {
	MemoryData memData{
		.rowPitch = rowPitch,
		.height = height,
		.src = srcMemoryRef,
		.dst = dstMemoryRef,
		.texture = true
	};

	m_memoryData.emplace_back(memData);
}

void UploadContainer::CopyTexture(const MemoryData& memData) const noexcept {
	const size_t alignedRowPitch = Align(
		memData.rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
	);

	for (size_t row = 0; row < memData.height; ++row) {
		void* dst = reinterpret_cast<std::uint8_t*>(memData.dst) + (alignedRowPitch * row);
		void const* src =
			reinterpret_cast<std::uint8_t const*>(memData.src) + (memData.rowPitch * row);

		memcpy(dst, src, memData.rowPitch);
	}
}

void UploadContainer::CopyBuffer(const MemoryData& memData) const noexcept {
	memcpy(memData.dst, memData.src, memData.rowPitch);
}

void UploadContainer::CopyData(std::atomic_size_t& workCount) const noexcept {
	++workCount;

	Gaia::threadPool->SubmitWork(
		[&] {
			for (size_t index = 0u; index < std::size(m_memoryData); ++index) {
				const MemoryData& memoryData = m_memoryData[index];

				if (memoryData.texture)
					CopyTexture(memoryData);
				else
					CopyBuffer(memoryData);
			}

			--workCount;
		}
	);
}
