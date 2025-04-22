#ifndef D3D_REUSABLE_BUFFER_HPP_
#define D3D_REUSABLE_BUFFER_HPP_
#include <ReusableVector.hpp>
#include <D3DResources.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <utility>
#include <concepts>

namespace Gaia
{
template<class Derived, class T>
class ReusableD3DBuffer
{
public:
	ReusableD3DBuffer(
		ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType
	) : m_buffers{ device, memoryManager, memoryType }, m_elements{}
	{}

	template<typename U>
	[[nodiscard]]
	// Returns the index of the element in the ElementBuffer.
	size_t Add(U&& element)
	{
		const size_t oldCount = m_elements.GetCount();

		const size_t extraElementCount
			= static_cast<Derived*>(this)->GetExtraElementAllocationCount();

		const size_t elementIndex = m_elements.Add(std::forward<U>(element), extraElementCount);

		const size_t newCount     = m_elements.GetCount();

		if(newCount > oldCount)
			static_cast<Derived*>(this)->CreateBuffer(newCount);

		return elementIndex;
	}

	[[nodiscard]]
	// Returns the indices of the elements in the ElementBuffer.
	std::vector<size_t> AddMultiple(std::vector<T>&& elements)
	{
		const size_t oldCount = m_elements.GetCount();

		const size_t extraElementCount
			= static_cast<Derived*>(this)->GetExtraElementAllocationCount();

		const size_t elementCount = std::size(elements);

		std::vector<size_t> elementIndices{};
		elementIndices.reserve(elementCount);

		for (T& element : elements)
		{
			const size_t elementIndex = m_elements.Add(std::move(element), extraElementCount);

			elementIndices.emplace_back(elementIndex);
		}

		const size_t newCount = m_elements.GetCount();

		if(newCount > oldCount)
			static_cast<Derived*>(this)->CreateBuffer(newCount);

		return elementIndices;
	}

	[[nodiscard]]
	// Returns the indices of the elements in the ElementBuffer.
	std::vector<std::uint32_t> AddMultipleRU32(std::vector<T>&& elements)
	{
		const size_t oldCount = m_elements.GetCount();

		const size_t extraElementCount
			= static_cast<Derived*>(this)->GetExtraElementAllocationCount();

		const size_t elementCount = std::size(elements);

		std::vector<std::uint32_t> elementIndices{};
		elementIndices.reserve(elementCount);

		for (T& element : elements)
		{
			const size_t elementIndex = m_elements.Add(std::move(element), extraElementCount);

			elementIndices.emplace_back(static_cast<std::uint32_t>(elementIndex));
		}

		const size_t newCount = m_elements.GetCount();

		if(newCount > oldCount)
			static_cast<Derived*>(this)->CreateBuffer(newCount);

		return elementIndices;
	}

	void Remove(size_t index) noexcept { m_elements.RemoveElement(index); }

protected:
	[[nodiscard]]
	size_t GetCount() const noexcept { return m_elements.GetCount(); }

protected:
	Buffer                      m_buffers;
	Callisto::ReusableVector<T> m_elements;

public:
	ReusableD3DBuffer(const ReusableD3DBuffer&) = delete;
	ReusableD3DBuffer& operator=(const ReusableD3DBuffer&) = delete;

	ReusableD3DBuffer(ReusableD3DBuffer&& other) noexcept
		: m_buffers{ std::move(other.m_buffers) }, m_elements{ std::move(other.m_elements) }
	{}
	ReusableD3DBuffer& operator=(ReusableD3DBuffer&& other) noexcept
	{
		m_buffers  = std::move(other.m_buffers);
		m_elements = std::move(other.m_elements);

		return *this;
	}
};

template<typename T>
class ReusableCPUBuffer
{
private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(T); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t elementCount)
	{
		constexpr size_t strideSize = GetStride();
		const auto buffersSize      = static_cast<UINT64>(strideSize * elementCount);

		Buffer newBuffer = GetCPUResource<Buffer>(m_device, m_memoryManager);
		newBuffer.Create(buffersSize, D3D12_RESOURCE_STATE_GENERIC_READ);

		const UINT64 oldBufferSize = m_buffer.BufferSize();

		if (oldBufferSize)
			memcpy(newBuffer.CPUHandle(), m_buffer.CPUHandle(), m_buffer.BufferSize());

		m_buffer = std::move(newBuffer);
	}

	void CreateBufferIfNecessary(size_t index)
	{
		const UINT64 currentSize    = m_buffer.BufferSize();
		constexpr size_t strideSize = GetStride();

		const auto minimumSpaceRequirement = static_cast<UINT64>(index * strideSize + strideSize);

		if (currentSize < minimumSpaceRequirement)
			CreateBuffer(index + 1u + GetExtraElementAllocationCount());
	}

public:
	ReusableCPUBuffer(ID3D12Device* device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_buffer{ GetCPUResource<Buffer>(device, memoryManager) }
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

	void Update(size_t index, const T& value)
	{
		std::uint8_t* bufferOffsetPtr = m_buffer.CPUHandle();
		constexpr size_t strideSize   = GetStride();
		const size_t bufferOffset     = index * strideSize;

		memcpy(bufferOffsetPtr + bufferOffset, &value, strideSize);
	}

	void Add(size_t index, const T& value)
	{
		CreateBufferIfNecessary(index);

		Update(index, value);
	}

private:
	ID3D12Device*  m_device;
	MemoryManager* m_memoryManager;
	Buffer         m_buffer;

public:
	ReusableCPUBuffer(const ReusableCPUBuffer&) = delete;
	ReusableCPUBuffer& operator=(const ReusableCPUBuffer&) = delete;

	ReusableCPUBuffer(ReusableCPUBuffer&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_buffer{ std::move(other.m_buffer) }
	{}
	ReusableCPUBuffer& operator=(ReusableCPUBuffer&& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = other.m_memoryManager;
		m_buffer        = std::move(other.m_buffer);

		return *this;
	}
};

template<typename T>
class MultiInstanceCPUBuffer
{
private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(T); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t elementCount)
	{
		constexpr size_t strideSize = GetStride();
		m_instanceSize              = static_cast<UINT64>(strideSize * elementCount);
		const UINT64 buffersSize    = m_instanceSize * m_instanceCount;

		m_buffer.Create(buffersSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

public:
	MultiInstanceCPUBuffer(
		ID3D12Device* device, MemoryManager* memoryManager, std::uint32_t instanceCount
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_buffer{ GetCPUResource<Buffer>(device, memoryManager) },
		m_instanceSize{ 0u }, m_instanceCount{ instanceCount }
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

	void ExtendBufferIfNecessaryFor(size_t index)
	{
		const UINT64 currentSize    = m_buffer.BufferSize();
		constexpr size_t strideSize = GetStride();

		const auto minimumSpaceRequirement = static_cast<UINT64>(index * strideSize + strideSize);

		if (currentSize < minimumSpaceRequirement)
			CreateBuffer(index + 1u + GetExtraElementAllocationCount());
	}

private:
	ID3D12Device*  m_device;
	MemoryManager* m_memoryManager;
	Buffer         m_buffer;
	UINT64         m_instanceSize;
	std::uint32_t  m_instanceCount;

public:
	MultiInstanceCPUBuffer(const MultiInstanceCPUBuffer&) = delete;
	MultiInstanceCPUBuffer& operator=(const MultiInstanceCPUBuffer&) = delete;

	MultiInstanceCPUBuffer(MultiInstanceCPUBuffer&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_buffer{ std::move(other.m_buffer) },
		m_instanceSize{ other.m_instanceSize }, m_instanceCount{other.m_instanceCount}
	{}
	MultiInstanceCPUBuffer& operator=(MultiInstanceCPUBuffer&& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = other.m_memoryManager;
		m_buffer        = std::move(other.m_buffer);
		m_instanceSize  = other.m_instanceSize;
		m_instanceCount = other.m_instanceCount;

		return *this;
	}
};
}
#endif
