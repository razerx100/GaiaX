#ifndef	STAGING_BUFFER_MANAGER_HPP_
#define STAGING_BUFFER_MANAGER_HPP_
#include <D3DResources.hpp>
#include <D3DCommandQueue.hpp>
#include <vector>
#include <ThreadPool.hpp>
#include <TemporaryDataBuffer.hpp>

class StagingBufferManager
{
public:
	StagingBufferManager(
		ID3D12Device* device, MemoryManager* memoryManager, ThreadPool* threadPool
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_threadPool{ threadPool }, m_bufferInfo{}, m_tempBufferToBuffer{},
		m_textureInfo{}, m_tempBufferToTexture{}, m_cpuTempBuffer{}
	{}

	// The destination info is required, when an ownership transfer is desired. Which
	// is needed when a resource has exclusive ownership.
	StagingBufferManager& AddTexture(
		std::shared_ptr<void> cpuData, Texture const* dst,
		TemporaryDataBufferGPU& tempDataBuffer, UINT mipLevelIndex = 0u
	);
	StagingBufferManager& AddBuffer(
		std::shared_ptr<void> cpuData, UINT64 bufferSize, Buffer const* dst, UINT64 offset,
		TemporaryDataBufferGPU& tempDataBuffer
	);

	void CopyAndClear(const D3DCommandList& copyCmdList);

private:
	void CopyCPU();
	void CopyGPU(const D3DCommandList& copyCmdList);

	void CleanUpTempBuffers() noexcept;
	void CleanUpBufferInfo() noexcept;

private:
	struct BufferInfo
	{
		void const*   cpuHandle;
		UINT64        bufferSize;
		Buffer const* dst;
		UINT64        offset;
	};

	struct TextureInfo
	{
		void const*    cpuHandle;
		UINT64         bufferSize;
		Texture const* dst;
		UINT           mipLevelIndex;
	};

private:
	ID3D12Device*                        m_device;
	MemoryManager*                       m_memoryManager;
	ThreadPool*                          m_threadPool;
	std::vector<BufferInfo>              m_bufferInfo;
	std::vector<std::shared_ptr<Buffer>> m_tempBufferToBuffer;
	std::vector<TextureInfo>             m_textureInfo;
	std::vector<std::shared_ptr<Buffer>> m_tempBufferToTexture;
	TemporaryDataBufferCPU               m_cpuTempBuffer;

public:
	StagingBufferManager(const StagingBufferManager&) = delete;
	StagingBufferManager& operator=(const StagingBufferManager&) = delete;

	StagingBufferManager(StagingBufferManager&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_threadPool{ other.m_threadPool },
		m_bufferInfo{ std::move(other.m_bufferInfo) },
		m_tempBufferToBuffer{ std::move(other.m_tempBufferToBuffer) },
		m_textureInfo{ std::move(other.m_textureInfo) },
		m_tempBufferToTexture{ std::move(other.m_tempBufferToTexture) },
		m_cpuTempBuffer{ std::move(other.m_cpuTempBuffer) }
	{}

	StagingBufferManager& operator=(StagingBufferManager&& other) noexcept
	{
		m_device              = other.m_device;
		m_memoryManager       = other.m_memoryManager;
		m_threadPool          = other.m_threadPool;
		m_bufferInfo          = std::move(other.m_bufferInfo);
		m_tempBufferToBuffer  = std::move(other.m_tempBufferToBuffer);
		m_textureInfo         = std::move(other.m_textureInfo);
		m_tempBufferToTexture = std::move(other.m_tempBufferToTexture);
		m_cpuTempBuffer       = std::move(other.m_cpuTempBuffer);

		return *this;
	}
};
#endif
