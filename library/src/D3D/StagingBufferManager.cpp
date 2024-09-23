#include <StagingBufferManager.hpp>
#include <ranges>
#include <algorithm>
#include <cassert>

StagingBufferManager& StagingBufferManager::AddTexture(
	std::shared_ptr<void> cpuData, Texture const* dst,
	TemporaryDataBufferGPU& tempDataBuffer, UINT mipLevelIndex/* = 0u */
) {
	const UINT64 bufferSize = dst->GetBufferSize();

	m_textureInfo.emplace_back(
		TextureInfo{
			.cpuHandle     = cpuData.get(),
			.bufferSize    = bufferSize,
			.dst           = dst,
			.mipLevelIndex = mipLevelIndex
		}
	);

	m_cpuTempBuffer.Add(std::move(cpuData));

	auto tempBuffer = std::make_shared<Buffer>(m_device, m_memoryManager, D3D12_HEAP_TYPE_UPLOAD);

	m_tempBufferToTexture.emplace_back(tempBuffer);

	tempBuffer->Create(bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);

	tempDataBuffer.Add(std::move(tempBuffer));

	return *this;
}

StagingBufferManager& StagingBufferManager::AddBuffer(
	std::shared_ptr<void> cpuData, UINT64 bufferSize, Buffer const* dst, UINT64 offset,
	TemporaryDataBufferGPU& tempDataBuffer
) {
	m_bufferInfo.emplace_back(
		BufferInfo{
			.cpuHandle  = cpuData.get(),
			.bufferSize = bufferSize,
			.dst        = dst,
			.offset     = offset
		}
	);

	m_cpuTempBuffer.Add(std::move(cpuData));

	auto tempBuffer = std::make_shared<Buffer>(m_device, m_memoryManager, D3D12_HEAP_TYPE_UPLOAD);

	m_tempBufferToBuffer.emplace_back(tempBuffer);

	tempBuffer->Create(bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);

	tempDataBuffer.Add(std::move(tempBuffer));

	return *this;
}

void StagingBufferManager::CopyCPU()
{
	struct Batch
	{
		size_t startIndex;
		size_t endIndex;
	};

	// Making these static, so dynamic allocation happens less.
	static std::vector<std::future<void>>     waitObjs{};
	static std::vector<std::function<void()>> tasks{};
	static std::vector<Batch>                 batches{};

	static constexpr size_t batchSize = 250_MB;

	Batch tempBatch{ .startIndex = 0u, .endIndex = 0u };
	size_t currentBatchSize = 0u;

	{
		for (size_t index = 0u; index < std::size(m_bufferInfo); ++index)
		{
			const BufferInfo& bufferInfo = m_bufferInfo[index];

			tasks.emplace_back([&bufferInfo, &tempBuffer = m_tempBufferToBuffer[index]]
				{
					memcpy(tempBuffer->CPUHandle(), bufferInfo.cpuHandle, bufferInfo.bufferSize);
				});

			currentBatchSize += bufferInfo.bufferSize;

			if (currentBatchSize >= batchSize)
			{
				currentBatchSize   = 0u;

				tempBatch.endIndex = index;
				batches.emplace_back(tempBatch);

				tempBatch.startIndex = index + 1u;
				tempBatch.endIndex   = tempBatch.startIndex;
			}
		}

		// No need to finish the batching here since we might be able to put some textures into it.
	}

	{
		// Need an offset here, since I want to put both buffers and textures into the same batch.
		const size_t offset = std::size(m_bufferInfo);

		for (size_t index = 0u; index < std::size(m_textureInfo); ++index)
		{
			const TextureInfo& textureInfo = m_textureInfo[index];

			tasks.emplace_back([&textureInfo, &tempBuffer = m_tempBufferToTexture[index]]
				{
					// The RowPitch in a texture which was loaded from the Disk Drive won't have its
					// rowPitch aligned. But the D3D textures need their rowPitches to be aligned to 256B.
					// So, the textures need to be copied like this.
					Texture const* texture = textureInfo.dst;

					const auto rowCount    = static_cast<size_t>(texture->GetHeight());
					const auto srcRowPitch = static_cast<size_t>(texture->GetRowPitch());
					const auto dstRowPitch = static_cast<size_t>(texture->GetRowPitchD3DAligned());

					std::uint8_t* dst = tempBuffer->CPUHandle();
					auto src          = static_cast<std::uint8_t const*>(textureInfo.cpuHandle);

					size_t srcOffset = 0u;
					size_t dstOffset = 0u;

					for (size_t rowIndex = 0u; rowIndex < rowCount; ++rowIndex)
					{
						memcpy(dst + dstOffset, src + srcOffset, srcRowPitch);

						srcOffset += srcRowPitch;
						dstOffset += dstRowPitch;
					}
				});

			currentBatchSize += textureInfo.bufferSize;

			if (currentBatchSize >= batchSize)
			{
				currentBatchSize   = 0u;

				tempBatch.endIndex = offset + index;
				batches.emplace_back(tempBatch);

				tempBatch.startIndex = offset + index + 1u;
				tempBatch.endIndex   = tempBatch.startIndex;
			}
		}
	}

	// Add the last element in the batch.
	if (auto elementCount = std::size(m_textureInfo) + std::size(m_bufferInfo);
		elementCount > tempBatch.endIndex)
	{
		tempBatch.endIndex = elementCount - 1u;
		batches.emplace_back(tempBatch);
	}

	// Copy batches.
	// This is asynchronous but it should be fine as different threads shouldn't access the same
	// location.
	for (auto& batch : batches)
	{
		waitObjs.emplace_back(m_threadPool->SubmitWork(std::function{
			[&tTasks = tasks, batch]
			{
				for (size_t index = batch.startIndex; index <= batch.endIndex; ++index)
				{
					tTasks[index]();
				}
			}}));
	}

	// Wait for all the copying to finish.
	for (auto& waitObj : waitObjs)
		waitObj.wait();

	// Only clearing, so we might not need to do dynamic allocate the next time.
	waitObjs.clear();
	tasks.clear();
	batches.clear();
}

void StagingBufferManager::CopyGPU(const D3DCommandList& copyCmdList)
{
	// Assuming the command buffer has been reset before this.
	for(size_t index = 0u; index < std::size(m_bufferInfo); ++index)
	{
		const BufferInfo& bufferInfo = m_bufferInfo[index];
		const Buffer& tempBuffer     = *m_tempBufferToBuffer[index];

		// I am making a new buffer for each copy but if the buffer alignment for example is
		// 16 bytes and the buffer size is 4bytes, copyWhole would be wrong as it would go over
		// the reserved size in the destination buffer.
		copyCmdList.Copy(
			tempBuffer, 0u, *bufferInfo.dst, bufferInfo.offset, bufferInfo.bufferSize
		);
	}

	for(size_t index = 0u; index < std::size(m_textureInfo); ++index)
	{
		const TextureInfo& textureInfo = m_textureInfo[index];
		const Buffer& tempBuffer       = *m_tempBufferToTexture[index];

		// CopyWhole would not be a problem for textures, as the destination buffer would be a texture
		// and will be using the dimension of the texture instead of its size to copy. And there should
		// be only a single texture in a texture buffer.
		copyCmdList.CopyWhole(tempBuffer, *textureInfo.dst, textureInfo.mipLevelIndex);
	}
}

void StagingBufferManager::CopyAndClear(const D3DCommandList& copyCmdList)
{
	// Since these are first copied to temp buffers and those are
	// copied on the GPU, we don't need any cpu synchronisation.
	// But we should wait on some semaphores from other queues which
	// are already running before we submit these copy commands.
	if (!std::empty(m_textureInfo) || !std::empty(m_bufferInfo))
	{
		CopyCPU();
		CopyGPU(copyCmdList);

		// Now that the cpu copying is done. We can clear the tempData.
		m_cpuTempBuffer.Clear();

		// It's okay to clean the Temp buffers up here. As we have another instance in the
		// global tempBuffer and that one should be deleted after the resources have been
		// copied on the GPU.
		CleanUpTempBuffers();
		CleanUpBufferInfo();
	}
}

void StagingBufferManager::CleanUpTempBuffers() noexcept
{
	m_tempBufferToBuffer.clear();
	m_tempBufferToTexture.clear();
}

void StagingBufferManager::CleanUpBufferInfo() noexcept
{
	m_bufferInfo.clear();
	m_textureInfo.clear();
}
