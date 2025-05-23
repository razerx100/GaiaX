#ifndef D3D_MULTI_INSTANCE_BUFFER_HPP_
#define D3D_MULTI_INSTANCE_BUFFER_HPP_
#include <D3DResources.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <utility>
#include <concepts>

namespace Gaia
{
class MultiInstanceCPUBuffer
{
public:
	MultiInstanceCPUBuffer(
		ID3D12Device* device, MemoryManager* memoryManager, std::uint32_t instanceCount,
		std::uint32_t strideSize
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_buffer{ GetCPUResource<Buffer>(device, memoryManager) },
		m_instanceSize{ 0u }, m_instanceCount{ instanceCount }, m_strideSize{ strideSize }
	{}

	void SetRootSRVGfx(
		D3DDescriptorManager& descriptorManager, size_t registerSlot , size_t registerSpace
	) const {
		descriptorManager.SetRootSRV(registerSlot, registerSpace, m_buffer.GetGPUAddress(), true);
	}
	void SetRootSRVCom(
		D3DDescriptorManager& descriptorManager, size_t registerSlot , size_t registerSpace
	) const {
		descriptorManager.SetRootSRV(registerSlot, registerSpace, m_buffer.GetGPUAddress(), false);
	}
	void SetRootCBVGfx(
		D3DDescriptorManager& descriptorManager, size_t registerSlot , size_t registerSpace
	) const {
		descriptorManager.SetRootCBV(registerSlot, registerSpace, m_buffer.GetGPUAddress(), true);
	}
	void SetRootCBVCom(
		D3DDescriptorManager& descriptorManager, size_t registerSlot , size_t registerSpace
	) const {
		descriptorManager.SetRootCBV(registerSlot, registerSpace, m_buffer.GetGPUAddress(), false);
	}

	std::uint8_t* GetInstancePtr(UINT64 instanceIndex) const noexcept
	{
		return m_buffer.CPUHandle() + (m_instanceSize * instanceIndex);
	}

	void AllocateForIndex(size_t index)
	{
		const UINT64 currentSize = m_buffer.BufferSize();
		const size_t strideSize  = GetStride();

		// The index of the first element will be 0, so need to add the space for an
		// extra element.
		const auto minimumSpaceRequirement = static_cast<UINT64>(index * strideSize + strideSize);

		if (currentSize < minimumSpaceRequirement)
			CreateBuffer(index + 1u + GetExtraElementAllocationCount());
	}

private:
	[[nodiscard]]
	std::uint32_t GetStride() const noexcept { return m_strideSize; }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t elementCount)
	{
		const size_t strideSize  = GetStride();
		m_instanceSize           = static_cast<UINT64>(strideSize * elementCount);
		const UINT64 buffersSize = m_instanceSize * m_instanceCount;

		m_buffer.Create(buffersSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

private:
	ID3D12Device*  m_device;
	MemoryManager* m_memoryManager;
	Buffer         m_buffer;
	UINT64         m_instanceSize;
	std::uint32_t  m_instanceCount;
	// Hopefully the stride won't be bigger than 4GB?
	std::uint32_t  m_strideSize;

public:
	MultiInstanceCPUBuffer(const MultiInstanceCPUBuffer&) = delete;
	MultiInstanceCPUBuffer& operator=(const MultiInstanceCPUBuffer&) = delete;

	MultiInstanceCPUBuffer(MultiInstanceCPUBuffer&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ other.m_memoryManager },
		m_buffer{ std::move(other.m_buffer) },
		m_instanceSize{ other.m_instanceSize },
		m_instanceCount{ other.m_instanceCount },
		m_strideSize{ other.m_strideSize }
	{}
	MultiInstanceCPUBuffer& operator=(MultiInstanceCPUBuffer&& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = other.m_memoryManager;
		m_buffer        = std::move(other.m_buffer);
		m_instanceSize  = other.m_instanceSize;
		m_instanceCount = other.m_instanceCount;
		m_strideSize    = other.m_strideSize;

		return *this;
	}
};
}
#endif
