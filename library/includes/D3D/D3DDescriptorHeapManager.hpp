#ifndef D3D_DESCRIPTOR_HEAP_MANAGER_HPP_
#define D3D_DESCRIPTOR_HEAP_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <utility>
#include <IndicesManager.hpp>
#include <D3DDescriptorLayout.hpp>
#include <memory>

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

class D3DDescriptorMap
{
public:
	D3DDescriptorMap() : m_singleDescriptors{}, m_descriptorTables{} {}

	D3DDescriptorMap& AddCBVGfx(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;
	D3DDescriptorMap& AddCBVCom(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;

	D3DDescriptorMap& AddUAVGfx(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;
	D3DDescriptorMap& AddUAVCom(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;

	D3DDescriptorMap& AddSRVGfx(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;
	D3DDescriptorMap& AddSRVCom(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;

	D3DDescriptorMap& AddDescTableGfx(UINT rootIndex, UINT descriptorIndex) noexcept;
	D3DDescriptorMap& AddDescTableCom(UINT rootIndex, UINT descriptorIndex) noexcept;

	void Bind(const D3DDescriptorHeap& descriptorHeap, ID3D12GraphicsCommandList* commandList) const;

private:
	template<void (ID3D12GraphicsCommandList::*bindViewFunction)(UINT, D3D12_GPU_VIRTUAL_ADDRESS)>
	static void ProxyView(
		ID3D12GraphicsCommandList* commandList, UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
	) {
		(commandList->*bindViewFunction)(rootIndex, bufferAddress);
	}
	template<void (ID3D12GraphicsCommandList::*bindTableFunction)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)>
	static void ProxyTable(
		ID3D12GraphicsCommandList* commandList, UINT rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle
	) {
		(commandList->*bindTableFunction)(rootIndex, gpuHandle);
	}

	struct SingleDescriptorMap
	{
		UINT                      rootIndex;
		D3D12_GPU_VIRTUAL_ADDRESS bufferAddress;
		void(*bindViewFunction)(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_VIRTUAL_ADDRESS);
	};

	struct DescriptorTableMap
	{
		UINT rootIndex;
		UINT descriptorIndex;
		void(*bindTableFunction)(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE);
	};

private:
	std::vector<SingleDescriptorMap> m_singleDescriptors;
	std::vector<DescriptorTableMap>  m_descriptorTables;

public:
	D3DDescriptorMap(const D3DDescriptorMap& other) noexcept
		: m_singleDescriptors{ other.m_singleDescriptors },
		m_descriptorTables{ other.m_descriptorTables }
	{}
	D3DDescriptorMap& operator=(const D3DDescriptorMap& other) noexcept
	{
		m_singleDescriptors = other.m_singleDescriptors;
		m_descriptorTables  = other.m_descriptorTables;

		return *this;
	}

	D3DDescriptorMap(D3DDescriptorMap&& other) noexcept
		: m_singleDescriptors{ std::move(other.m_singleDescriptors) },
		m_descriptorTables{ std::move(other.m_descriptorTables) }
	{}
	D3DDescriptorMap& operator=(D3DDescriptorMap&& other) noexcept
	{
		m_singleDescriptors = std::move(other.m_singleDescriptors);
		m_descriptorTables  = std::move(other.m_descriptorTables);

		return *this;
	}
};

class D3DDescriptorManager
{
public:
	D3DDescriptorManager(ID3D12Device* device, size_t layoutCount)
		: m_rtvHeap{ device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
		m_resourceHeapGPU{
			device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		}, m_descriptorMap{},
		m_dsvHeap{ device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE },
		m_resourceHeapCPU{
			device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		}, m_descriptorLayouts{ layoutCount, D3DDescriptorLayout{} }
	{}

	D3DDescriptorManager& AddCBV(
		size_t registerSlot, size_t registerSpace, UINT descriptorCount,
		D3D12_SHADER_VISIBILITY shaderStage
	) noexcept {
		m_descriptorLayouts.at(registerSpace).AddCBV(registerSlot, descriptorCount, shaderStage);

		return *this;
	}
	D3DDescriptorManager& AddSRV(
		size_t registerSlot, size_t registerSpace, UINT descriptorCount,
		D3D12_SHADER_VISIBILITY shaderStage
	) noexcept {
		m_descriptorLayouts.at(registerSpace).AddSRV(registerSlot, descriptorCount, shaderStage);

		return *this;
	}
	D3DDescriptorManager& AddUAV(
		size_t registerSlot, size_t registerSpace, UINT descriptorCount,
		D3D12_SHADER_VISIBILITY shaderStage
	) noexcept {
		m_descriptorLayouts.at(registerSpace).AddUAV(registerSlot, descriptorCount, shaderStage);

		return *this;
	}

	void CreateDescriptors();

	[[nodiscard]]
	D3DReusableDescriptorHeap& GetRTVHeap() noexcept { return m_rtvHeap; }
	[[nodiscard]]
	D3DReusableDescriptorHeap& GetDSVHeap() noexcept { return m_dsvHeap; }

	[[nodiscard]]
	const std::vector<D3DDescriptorLayout> GetLayouts() const noexcept { return m_descriptorLayouts; }

private:
	D3DReusableDescriptorHeap        m_rtvHeap;
	D3DDescriptorHeap                m_resourceHeapGPU;
	D3DDescriptorMap                 m_descriptorMap;
	D3DReusableDescriptorHeap        m_dsvHeap;
	D3DDescriptorHeap                m_resourceHeapCPU;
	std::vector<D3DDescriptorLayout> m_descriptorLayouts;

public:
	D3DDescriptorManager(const D3DDescriptorManager&) = delete;
	D3DDescriptorManager& operator=(const D3DDescriptorManager&) = delete;

	D3DDescriptorManager(D3DDescriptorManager&& other) noexcept
		: m_rtvHeap{ std::move(other.m_rtvHeap) },
		m_resourceHeapGPU{ std::move(other.m_resourceHeapGPU) },
		m_descriptorMap{ std::move(other.m_descriptorMap) },
		m_dsvHeap{ std::move(other.m_dsvHeap) },
		m_resourceHeapCPU{ std::move(other.m_resourceHeapCPU) },
		m_descriptorLayouts{ std::move(other.m_descriptorLayouts) }
	{}
	D3DDescriptorManager& operator=(D3DDescriptorManager&& other) noexcept
	{
		m_rtvHeap           = std::move(other.m_rtvHeap);
		m_resourceHeapGPU   = std::move(other.m_resourceHeapGPU);
		m_descriptorMap     = std::move(other.m_descriptorMap);
		m_dsvHeap           = std::move(other.m_dsvHeap);
		m_resourceHeapCPU   = std::move(other.m_resourceHeapCPU);
		m_descriptorLayouts = std::move(other.m_descriptorLayouts);

		return *this;
	}
};
#endif
