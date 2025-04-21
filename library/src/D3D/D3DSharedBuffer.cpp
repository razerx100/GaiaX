#include <ranges>
#include <algorithm>

#include <D3DSharedBuffer.hpp>

// Shared Buffer GPU
void SharedBufferGPU::CreateBuffer(UINT64 size, Callisto::TemporaryDataBufferGPU& tempBuffer)
{
	// Moving it into the old buffer, as we will want to copy it back to the new bigger buffer.

	// The temp old buffer should be null after a copy is done
	// and it has been destroyed. If it is at the beginning and the old buffer didn't have any
	// data, just moving wouldn't be an issue. But once it has some data, if we try to increase
	// the buffer size twice, it will move the old buffer with the data to the tempbuffer, but
	// if the GPU copy hasn't been done before it is increased again, the old tempBuffer will
	// be replaced with a new empty one and all data will be lost.
	// But this check should fix everything in theory. As, if we are recreating the main buffer
	// multiple times, the new old buffer would be empty as the gpu copy wouldn't have been done yet.
	// So, we wouldn't need to copy it. And since the first old buffer will be stored in the temp
	// buffer, it won't be null and so we wouldn't replace it and the data should be preserved and
	// safely copied to the main buffer upon calling CopyOldBuffer next.
	// Also no need to copy if the main buffer doesn't exist.

	// To add, if the buffer is increased twice in the beginning, before a copy is made, the
	// old temp buffer will be empty. And so we have to copy the old buffer first on the gpu.
	// Checking if that range is copied would also work, but this should only happen once, so
	// one empty copy should be fine.
	if (m_buffer.Get() && !m_oldBuffer)
	{
		m_oldBuffer = std::make_shared<Buffer>(std::move(m_buffer));
		tempBuffer.Add(m_oldBuffer);
	}

	m_buffer = GetGPUResource<Buffer>(m_device, m_memoryManager);
	m_buffer.Create(size, m_resourceState);
}

UINT64 SharedBufferGPU::ExtendBuffer(UINT64 size, Callisto::TemporaryDataBufferGPU& tempBuffer)
{
	// I probably don't need to worry about aligning here, since it's all inside a single buffer?
	const UINT64 oldSize = m_buffer.BufferSize();
	const UINT64 offset  = oldSize;
	const UINT64 newSize = oldSize + size;

	// If the alignment is 16bytes, at least 16bytes will be allocated. If the requested size
	// is bigger, then there shouldn't be any issues. But if the requested size is smaller,
	// the offset would be correct, but the buffer would be unnecessarily recreated, even though
	// it is not necessary. So, putting a check here.
	if (newSize > oldSize)
		CreateBuffer(newSize, tempBuffer);

	return offset;
}

void SharedBufferGPU::CopyOldBuffer(const D3DCommandList& copyList) noexcept
{
	if (m_oldBuffer)
	{
		// The temp buffer should get promoted to COPY_SRC and the new buffer
		// to COPY_DST and then get decayed to COMMON afterwards and then
		// to the desired state.
		std::shared_ptr<Buffer> oldBuffer = std::move(m_oldBuffer);
		copyList.CopyWhole(*oldBuffer, m_buffer);
	}
}

SharedBufferData SharedBufferGPU::AllocateAndGetSharedData(
	UINT64 size, Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	auto availableAllocIndex = m_allocator.GetAvailableAllocInfo(size);

	Callisto::SharedBufferAllocator::AllocInfo allocInfo{ .offset = 0u, .size = 0u };

	if (!availableAllocIndex)
	{
		allocInfo.size   = size;
		allocInfo.offset = ExtendBuffer(size, tempBuffer);
	}
	else
		allocInfo = m_allocator.GetAndRemoveAllocInfo(*availableAllocIndex);

	return SharedBufferData{
		.bufferData = &m_buffer,
		.offset     = m_allocator.AllocateMemory(allocInfo, size),
		.size       = size
	};
}
