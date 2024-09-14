#include <ranges>
#include <algorithm>

#include <CommonBuffers.hpp>

// Material Buffers
void MaterialBuffers::SetDescriptorLayout(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t registerSlot, size_t registerSpace
) const noexcept {
	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
		descriptorManager.AddRootSRV(registerSlot, registerSpace, D3D12_SHADER_VISIBILITY_PIXEL);
}

void MaterialBuffers::SetDescriptor(
	std::vector<D3DDescriptorManager>& descriptorManagers, size_t registerSlot, size_t registerSpace
) const {
	for (D3DDescriptorManager& descriptorManager : descriptorManagers)
		descriptorManager.SetRootSRV(registerSlot, registerSpace, m_buffers.GetGPUAddress(), true);
}

void MaterialBuffers::CreateBuffer(size_t materialCount)
{
	constexpr size_t strideSize    = GetStride();
	const auto materialBuffersSize = static_cast<UINT64>(strideSize * materialCount);

	Buffer newBuffer = GetCPUResource<Buffer>(m_device, m_memoryManager);
	newBuffer.Create(materialBuffersSize, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// All of the old materials will be only copied if the new buffer is larger.
	const UINT64 oldBufferSize = m_buffers.BufferSize();
	if (oldBufferSize && newBuffer.BufferSize() > oldBufferSize)
		memcpy(newBuffer.CPUHandle(), m_buffers.CPUHandle(), m_buffers.BufferSize());

	m_buffers = std::move(newBuffer);
}

void MaterialBuffers::Update(size_t index) const noexcept
{
	std::uint8_t* bufferOffset  = m_buffers.CPUHandle();
	constexpr size_t strideSize = GetStride();
	size_t materialOffset       = index * strideSize;

	if (m_elements.IsInUse(index))
	{
		const std::shared_ptr<Material>& material = m_elements.at(index);
		const MaterialData materialData           = material->Get();

		memcpy(bufferOffset + materialOffset, &materialData, strideSize);
	}
}

void MaterialBuffers::Update(const std::vector<size_t>& indices) const noexcept
{
	std::uint8_t* bufferOffset  = m_buffers.CPUHandle();
	constexpr size_t strideSize = GetStride();

	for (size_t index : indices)
	{
		if (m_elements.IsInUse(index))
		{
			const std::shared_ptr<Material>& material = m_elements.at(index);
			const MaterialData materialData           = material->Get();

			const size_t materialOffset = index * strideSize;
			memcpy(bufferOffset + materialOffset, &materialData, strideSize);
		}
	}
}

// Shared Buffer Allocator
void SharedBufferAllocator::AddAllocInfo(UINT64 offset, UINT64 size) noexcept
{
	auto result = std::ranges::upper_bound(
		m_availableMemory, size, {},
		[](const AllocInfo& info) { return info.size; }
	);

	m_availableMemory.insert(result, AllocInfo{ offset, size });
}

std::optional<size_t> SharedBufferAllocator::GetAvailableAllocInfo(UINT64 size) const noexcept
{
	auto result = std::ranges::lower_bound(
		m_availableMemory, size, {},
		[](const AllocInfo& info) { return info.size; }
	);

	if (result != std::end(m_availableMemory))
		return std::distance(std::begin(m_availableMemory), result);
	else
		return {};
}

SharedBufferAllocator::AllocInfo SharedBufferAllocator::GetAndRemoveAllocInfo(size_t index) noexcept
{
	AllocInfo allocInfo = m_availableMemory[index];

	m_availableMemory.erase(std::next(std::begin(m_availableMemory), index));

	return allocInfo;
}

UINT64 SharedBufferAllocator::AllocateMemory(const AllocInfo& allocInfo, UINT64 size) noexcept
{
	const UINT64 offset     = allocInfo.offset;
	const UINT64 freeMemory = allocInfo.size - size;

	if (freeMemory)
		AddAllocInfo(offset + size, freeMemory);

	return offset;
}

// Shared Buffer GPU
void SharedBufferGPU::CreateBuffer(UINT64 size, TemporaryDataBufferGPU& tempBuffer)
{
	// Moving it into the temp, as we will want to copy it back to the new bigger buffer.

	// This part is really important. So, the temp buffer should be null after a copy is done
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
	if (m_buffer.Get() != nullptr && m_tempBuffer == nullptr)
	{
		m_tempBuffer = std::make_shared<Buffer>(std::move(m_buffer));
		tempBuffer.Add(m_tempBuffer);
	}

	m_buffer = GetGPUResource<Buffer>(m_device, m_memoryManager);
	m_buffer.Create(size, m_resourceState);
}

UINT64 SharedBufferGPU::ExtendBuffer(UINT64 size, TemporaryDataBufferGPU& tempBuffer)
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
	if (m_tempBuffer)
	{
		// The temp buffer should get promoted to COPY_SRC and the new buffer
		// to COPY_DST and then get decayed to COMMON afterwards and then
		// to the desired state.
		std::shared_ptr<Buffer> tempBuffer = std::move(m_tempBuffer);
		copyList.CopyWhole(*tempBuffer, m_buffer);
	}
}

SharedBufferData SharedBufferGPU::AllocateAndGetSharedData(
	UINT64 size, TemporaryDataBufferGPU& tempBuffer
) {
	auto availableAllocIndex = m_allocator.GetAvailableAllocInfo(size);
	SharedBufferAllocator::AllocInfo allocInfo{ .offset = 0u, .size = 0u };

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
