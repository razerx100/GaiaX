#ifndef D3D_DESCRIPTOR_HEAP_MANAGER_HPP_
#define D3D_DESCRIPTOR_HEAP_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <utility>
#include <IndicesManager.hpp>

class D3DDescriptorHeap
{
public:
	D3DDescriptorHeap(
		ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag
	);

	void Create(UINT descriptorCount);

	// The heap type must be the same.
	void CopyHeap(
		const D3DDescriptorHeap& src, UINT descriptorCount, UINT srcOffset, UINT dstOffset
	);

	void Bind(ID3D12GraphicsCommandList* commandList) const noexcept;

	[[nodiscard]]
	D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const noexcept { return m_descriptorDesc.Type; }
	[[nodiscard]]
	UINT GetDescriptorCount() const noexcept { return m_descriptorDesc.NumDescriptors; }

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const noexcept;
	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index) const noexcept;
	[[nodiscard]]
	UINT GetDescriptorSize() const noexcept { return m_descriptorSize; }
	[[nodiscard]]
	ID3D12DescriptorHeap* Get() const noexcept { return m_descriptorHeap.Get(); }

private:
	ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
	UINT                         m_descriptorSize;
	D3D12_DESCRIPTOR_HEAP_DESC   m_descriptorDesc;
	ID3D12Device*                m_device;

public:
	D3DDescriptorHeap(const D3DDescriptorHeap&) = delete;
	D3DDescriptorHeap& operator=(const D3DDescriptorHeap&) = delete;

	D3DDescriptorHeap(D3DDescriptorHeap&& other) noexcept
		: m_descriptorHeap{ std::move(other.m_descriptorHeap) },
		m_descriptorSize{ other.m_descriptorSize },
		m_descriptorDesc{ other.m_descriptorDesc }, m_device{ other.m_device }
	{}
	D3DDescriptorHeap& operator=(D3DDescriptorHeap&& other) noexcept
	{
		m_descriptorHeap = std::move(other.m_descriptorHeap);
		m_descriptorSize = other.m_descriptorSize;
		m_descriptorDesc = other.m_descriptorDesc;
		m_device         = other.m_device;

		return *this;
	}
};

class D3DReusableDescriptorHeap
{
public:
	D3DReusableDescriptorHeap(
		ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag
	) : m_descriptorHeap{ device, type, flag }, m_device{ device }, m_indicesManager{}
	{}

	UINT CreateSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc);
	UINT CreateCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc);
	UINT CreateUAV(
		ID3D12Resource* resource, ID3D12Resource* counterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc
	);
	UINT CreateDSV(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc);
	UINT CreateRTV(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc);
	UINT CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc);

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const noexcept
	{
		return m_descriptorHeap.GetCPUHandle(index);
	}

	void FreeDescriptor(UINT index) noexcept
	{
		m_indicesManager.ToggleAvailability(index, true);
	}

private:
	inline static constexpr size_t s_extraAllocationCount = 4u;

private:
	[[nodiscard]]
	UINT GetNextFreeIndex(UINT extraAllocCount = 0);

	void ReserveNewElements(UINT newDescriptorCount);

	[[nodiscard]]
	UINT AllocateDescriptor();

private:
	D3DDescriptorHeap m_descriptorHeap;
	ID3D12Device*     m_device;
	IndicesManager    m_indicesManager;

public:
	D3DReusableDescriptorHeap(const D3DReusableDescriptorHeap&) = delete;
	D3DReusableDescriptorHeap& operator=(const D3DReusableDescriptorHeap&) = delete;

	D3DReusableDescriptorHeap(D3DReusableDescriptorHeap&& other) noexcept
		: m_descriptorHeap{ std::move(other.m_descriptorHeap) },
		m_device{ other.m_device }, m_indicesManager{ std::move(other.m_indicesManager) }
	{}
	D3DReusableDescriptorHeap& operator=(D3DReusableDescriptorHeap&& other) noexcept
	{
		m_descriptorHeap = std::move(other.m_descriptorHeap);
		m_device         = other.m_device;
		m_indicesManager = std::move(other.m_indicesManager);

		return *this;
	}
};
#endif
