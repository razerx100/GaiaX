#ifndef UPLOAD_CONTAINER_HPP_
#define UPLOAD_CONTAINER_HPP_
#include <memory>
#include <vector>
#include <atomic>

class UploadContainer {
public:
	void AddMemory(void const* srcMemoryRef, void* dstMemoryRef, size_t size) noexcept;
	void AddMemory(
		void const* srcMemoryRef, void* dstMemoryRef, size_t rowPitch, size_t height
	) noexcept;

	void CopyData(std::atomic_size_t& workCount) const noexcept;

private:
	struct MemoryData {
		size_t rowPitch;
		size_t height;
		void const* src;
		void* dst;
		bool texture;
	};

private:
	void CopyTexture(const MemoryData& memData) const noexcept;
	void CopyBuffer(const MemoryData& memData) const noexcept;

private:
	std::vector<MemoryData> m_memoryData;
};
#endif
