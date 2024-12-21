#ifndef COMMON_BUFFERS_HPP_
#define COMMON_BUFFERS_HPP_
#include <D3DResources.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <ReusableD3DBuffer.hpp>
#include <D3DCommandQueue.hpp>
#include <queue>
#include <TemporaryDataBuffer.hpp>

#include <Material.hpp>
#include <MeshBundle.hpp>

class MaterialBuffers : public ReusableD3DBuffer<MaterialBuffers, std::shared_ptr<Material>>
{
	friend class ReusableD3DBuffer<MaterialBuffers, std::shared_ptr<Material>>;
public:
	MaterialBuffers(ID3D12Device* device, MemoryManager* memoryManager)
		: ReusableD3DBuffer{ device, memoryManager, D3D12_HEAP_TYPE_UPLOAD },
		m_device{ device }, m_memoryManager{ memoryManager }
	{}

	void SetDescriptorLayout(
		std::vector<D3DDescriptorManager>& descriptorManagers, size_t registerSlot,
		size_t registerSpace
	) const noexcept;
	void SetDescriptor(
		std::vector<D3DDescriptorManager>& descriptorBuffers, size_t registerSlot,
		size_t registerSpace
	) const;

	// Shouldn't be called on every frame. Only updates the index specified.
	void Update(size_t index) const noexcept;
	// Shouldn't be called on every frame. Only updates the indices specified.
	void Update(const std::vector<size_t>& indices) const noexcept;

private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(MaterialData); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t materialCount);

private:
	ID3D12Device*  m_device;
	MemoryManager* m_memoryManager;

public:
	MaterialBuffers(const MaterialBuffers&) = delete;
	MaterialBuffers& operator=(const MaterialBuffers&) = delete;

	MaterialBuffers(MaterialBuffers&& other) noexcept
		: ReusableD3DBuffer{ std::move(other) },
		m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager }
	{}
	MaterialBuffers& operator=(MaterialBuffers&& other) noexcept
	{
		ReusableD3DBuffer::operator=(std::move(other));
		m_device                   = other.m_device;
		m_memoryManager            = other.m_memoryManager;

		return *this;
	}
};

struct SharedBufferData
{
	Buffer const* bufferData;
	UINT64        offset;
	UINT64        size;
};

class SharedBufferAllocator
{
public:
	struct AllocInfo
	{
		UINT64 offset;
		UINT64 size;
	};

public:
	SharedBufferAllocator() : m_availableMemory{} {}

	[[nodiscard]]
	// The offset from the start of the buffer will be returned. Should make sure
	// there is enough memory before calling this.
	UINT64 AllocateMemory(const AllocInfo& allocInfo, UINT64 size) noexcept;

	void AddAllocInfo(UINT64 offset, UINT64 size) noexcept;
	void RelinquishMemory(UINT64 offset, UINT64 size) noexcept
	{
		AddAllocInfo(offset, size);
	}

	[[nodiscard]]
	std::optional<size_t> GetAvailableAllocInfo(UINT64 size) const noexcept;
	[[nodiscard]]
	AllocInfo GetAndRemoveAllocInfo(size_t index) noexcept;

private:
	std::vector<AllocInfo> m_availableMemory;

public:
	SharedBufferAllocator(const SharedBufferAllocator& other) noexcept
		: m_availableMemory{ other.m_availableMemory }
	{}
	SharedBufferAllocator& operator=(const SharedBufferAllocator& other) noexcept
	{
		m_availableMemory = other.m_availableMemory;

		return *this;
	}
	SharedBufferAllocator(SharedBufferAllocator&& other) noexcept
		: m_availableMemory{ std::move(other.m_availableMemory) }
	{}
	SharedBufferAllocator& operator=(SharedBufferAllocator&& other) noexcept
	{
		m_availableMemory = std::move(other.m_availableMemory);

		return *this;
	}
};

class SharedBufferBase
{
protected:
	SharedBufferBase(
		ID3D12Device* device, MemoryManager* memoryManager, D3D12_RESOURCE_STATES resourceState,
		D3D12_RESOURCE_FLAGS buferFlag, Buffer&& buffer
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_buffer{ std::move(buffer) }, m_allocator{}, m_resourceState{ resourceState },
		m_bufferFlag{ buferFlag }
	{}

public:
	[[nodiscard]]
	UINT64 Size() const noexcept { return m_buffer.BufferSize(); }
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const noexcept
	{
		return m_buffer.GetGPUAddress();
	}

	[[nodiscard]]
	const Buffer& GetBuffer() const noexcept { return m_buffer; }

	[[nodiscard]]
	ID3D12Resource* GetD3Dbuffer() const noexcept { return m_buffer.Get(); }

protected:
	ID3D12Device*         m_device;
	MemoryManager*        m_memoryManager;
	Buffer                m_buffer;
	SharedBufferAllocator m_allocator;
	D3D12_RESOURCE_STATES m_resourceState;
	D3D12_RESOURCE_FLAGS  m_bufferFlag;

public:
	SharedBufferBase(const SharedBufferBase&) = delete;
	SharedBufferBase& operator=(const SharedBufferBase&) = delete;

	SharedBufferBase(SharedBufferBase&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_buffer{ std::move(other.m_buffer) },
		m_allocator{ std::move(other.m_allocator) },
		m_resourceState{ other.m_resourceState },
		m_bufferFlag{ other.m_bufferFlag }
	{}
	SharedBufferBase& operator=(SharedBufferBase&& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = other.m_memoryManager;
		m_buffer        = std::move(other.m_buffer);
		m_allocator     = std::move(other.m_allocator);
		m_resourceState = other.m_resourceState;
		m_bufferFlag    = other.m_bufferFlag;

		return *this;
	}
};

class SharedBufferGPU : public SharedBufferBase
{
public:
	SharedBufferGPU(
		ID3D12Device* device, MemoryManager* memoryManager,
		D3D12_RESOURCE_FLAGS bufferFlag = D3D12_RESOURCE_FLAG_NONE
	) : SharedBufferBase{
			// Since it is a gpu buffer and d3d12 has the resource promotion system
			// the buffer should get promoted to the desired state. Might need to do
			// manual transition in some cases though.
			device, memoryManager, D3D12_RESOURCE_STATE_COMMON, bufferFlag,
			GetGPUResource<Buffer>(device, memoryManager)
		}, m_oldBuffer{}
	{}

	void CopyOldBuffer(const D3DCommandList& copyList) noexcept;

	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	SharedBufferData AllocateAndGetSharedData(UINT64 size, TemporaryDataBufferGPU& tempBuffer);

	void RelinquishMemory(const SharedBufferData& sharedData) noexcept
	{
		m_allocator.RelinquishMemory(sharedData.offset, sharedData.size);
	}

private:
	void CreateBuffer(UINT64 size, TemporaryDataBufferGPU& tempBuffer);
	[[nodiscard]]
	UINT64 ExtendBuffer(UINT64 size, TemporaryDataBufferGPU& tempBuffer);

private:
	std::shared_ptr<Buffer> m_oldBuffer;

public:
	SharedBufferGPU(const SharedBufferGPU&) = delete;
	SharedBufferGPU& operator=(const SharedBufferGPU&) = delete;

	SharedBufferGPU(SharedBufferGPU&& other) noexcept
		: SharedBufferBase{ std::move(other) },
		m_oldBuffer{ std::move(other.m_oldBuffer) }
	{}
	SharedBufferGPU& operator=(SharedBufferGPU&& other) noexcept
	{
		SharedBufferBase::operator=(std::move(other));
		m_oldBuffer = std::move(other.m_oldBuffer);

		return *this;
	}
};

template<D3D12_HEAP_TYPE MemoryType>
class SharedBufferWriteOnly : public SharedBufferBase
{
public:
	SharedBufferWriteOnly(
		ID3D12Device* device, MemoryManager* memoryManager, D3D12_RESOURCE_STATES resourceState,
		D3D12_RESOURCE_FLAGS bufferFlag = D3D12_RESOURCE_FLAG_NONE
	) : SharedBufferBase{
			device, memoryManager, resourceState, bufferFlag,
			Buffer{ device, memoryManager, MemoryType }
		}
	{}

	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	SharedBufferData AllocateAndGetSharedData(UINT64 size, bool copyOldBuffer = false)
	{
		auto availableAllocIndex = m_allocator.GetAvailableAllocInfo(size);
		SharedBufferAllocator::AllocInfo allocInfo{ .offset = 0u, .size = 0u };

		if (!availableAllocIndex)
		{
			allocInfo.size   = size;
			allocInfo.offset = ExtendBuffer(size, copyOldBuffer);
		}
		else
			allocInfo = m_allocator.GetAndRemoveAllocInfo(*availableAllocIndex);

		return SharedBufferData{
			.bufferData = &m_buffer,
			.offset     = m_allocator.AllocateMemory(allocInfo, size),
			.size       = size
		};
	}

	void RelinquishMemory(const SharedBufferData& sharedData) noexcept
	{
		m_allocator.RelinquishMemory(sharedData.offset, sharedData.size);
	}

private:
	[[nodiscard]]
	UINT64 ExtendBuffer(UINT64 size, bool copyOldBuffer)
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
		{
			Buffer newBuffer{ m_device, m_memoryManager, MemoryType };

			newBuffer.Create(newSize, m_resourceState, m_bufferFlag);

			if (copyOldBuffer)
				memcpy(newBuffer.CPUHandle(), m_buffer.CPUHandle(), static_cast<size_t>(oldSize));

			m_buffer = std::move(newBuffer);
		}

		return offset;
	}

public:
	SharedBufferWriteOnly(const SharedBufferWriteOnly&) = delete;
	SharedBufferWriteOnly& operator=(const SharedBufferWriteOnly&) = delete;

	SharedBufferWriteOnly(SharedBufferWriteOnly&& other) noexcept
		: SharedBufferBase{ std::move(other) }
	{}
	SharedBufferWriteOnly& operator=(SharedBufferWriteOnly&& other) noexcept
	{
		SharedBufferBase::operator=(std::move(other));

		return *this;
	}
};

typedef SharedBufferWriteOnly<D3D12_HEAP_TYPE_UPLOAD> SharedBufferCPU;
typedef SharedBufferWriteOnly<D3D12_HEAP_TYPE_DEFAULT> SharedBufferGPUWriteOnly;
#endif
