#ifndef D3D_DESCRIPTOR_HEAP_MANAGER_HPP_
#define D3D_DESCRIPTOR_HEAP_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <ranges>
#include <algorithm>
#include <utility>
#include <IndicesManager.hpp>
#include <D3DDescriptorLayout.hpp>
#include <D3DCommandQueue.hpp>
#include <memory>
#include <optional>

class D3DDescriptorHeap
{
public:
	D3DDescriptorHeap(
		ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag
	);

	void Create(UINT descriptorCount);

	void CopyDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, UINT offset) const;
	// The heap type must be the same.
	void CopyDescriptors(
		const D3DDescriptorHeap& src, UINT descriptorCount, UINT srcOffset, UINT dstOffset
	) const;
	// The src heap must be CPU accessible.
	void CopyHeap(const D3DDescriptorHeap& src) const
	{
		CopyDescriptors(src, src.GetDescriptorCount(), 0u, 0u);
	}

	void Bind(ID3D12GraphicsCommandList* commandList) const noexcept;
	void Bind(const D3DCommandList& commandList) const noexcept { Bind(commandList.Get()); }

	void CreateSRV(
		ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, UINT descriptorIndex
	) const;
	void CreateCBV(
		const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc, UINT descriptorIndex
	) const;
	void CreateUAV(
		ID3D12Resource* resource, ID3D12Resource* counterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT descriptorIndex
	) const;
	void CreateDSV(
		ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, UINT descriptorIndex
	) const;
	void CreateRTV(
		ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, UINT descriptorIndex
	) const;
	void CreateSampler(
		const D3D12_SAMPLER_DESC& samplerDesc, UINT descriptorIndex
	) const;

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
	) : m_descriptorHeap{ device, type, flag }, m_indicesManager{}
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

	void CreateSRV(
		ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc,
		UINT descriptorIndex
	);
	void CreateCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc, UINT descriptorIndex);
	void CreateUAV(
		ID3D12Resource* resource, ID3D12Resource* counterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT descriptorIndex
	);
	void CreateDSV(
		ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, UINT descriptorIndex
	);
	void CreateRTV(
		ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, UINT descriptorIndex
	);
	void CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc, UINT descriptorIndex);

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
	D3DDescriptorHeap        m_descriptorHeap;
	Callisto::IndicesManager m_indicesManager;

public:
	D3DReusableDescriptorHeap(const D3DReusableDescriptorHeap&) = delete;
	D3DReusableDescriptorHeap& operator=(const D3DReusableDescriptorHeap&) = delete;

	D3DReusableDescriptorHeap(D3DReusableDescriptorHeap&& other) noexcept
		: m_descriptorHeap{ std::move(other.m_descriptorHeap) },
		m_indicesManager{ std::move(other.m_indicesManager) }
	{}
	D3DReusableDescriptorHeap& operator=(D3DReusableDescriptorHeap&& other) noexcept
	{
		m_descriptorHeap = std::move(other.m_descriptorHeap);
		m_indicesManager = std::move(other.m_indicesManager);

		return *this;
	}
};

class D3DDescriptorMap
{
public:
	D3DDescriptorMap() : m_rootDescriptors{}, m_descriptorTables{} {}

	D3DDescriptorMap& SetRootCBVGfx(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;
	D3DDescriptorMap& SetRootCBVCom(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;

	D3DDescriptorMap& SetRootUAVGfx(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;
	D3DDescriptorMap& SetRootUAVCom(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;

	D3DDescriptorMap& SetRootSRVGfx(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;
	D3DDescriptorMap& SetRootSRVCom(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress) noexcept;

	D3DDescriptorMap& SetDescTableGfx(UINT rootIndex, UINT descriptorIndex) noexcept;
	D3DDescriptorMap& SetDescTableCom(UINT rootIndex, UINT descriptorIndex) noexcept;

	void Bind(const D3DDescriptorHeap& descriptorHeap, ID3D12GraphicsCommandList* commandList) const;

	void Bind(
		const D3DDescriptorHeap& descriptorHeap, const D3DCommandList& commandList
	) const noexcept {
		Bind(descriptorHeap, commandList.Get());
	}

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

	template<typename T>
	[[nodiscard]]
	std::optional<size_t> FindRootIndex(UINT rootIndex) const noexcept
	{
		std::optional<size_t> result{};

		auto DescriptorsGetter = [this]<typename U>()-> const std::vector<U>&
		{
			if constexpr (std::is_same_v<U, SingleDescriptorMap>)
				return m_rootDescriptors;
			else
				return m_descriptorTables;
		};

		auto& descriptors = DescriptorsGetter.template operator()<T>();

		auto fResult = std::ranges::find(
            descriptors, rootIndex,
            [](const T& tableMap)
            {
                return tableMap.rootIndex;
            }
        );

        if (fResult != std::end(descriptors))
            result = std::distance(std::begin(descriptors), fResult);

		return result;
	}

	template<void(*BindViewFunction)(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_VIRTUAL_ADDRESS)>
	D3DDescriptorMap& SetRootDescriptor(
		UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferAddress
	) noexcept {
		std::optional<size_t> rootIndexLocation = FindRootIndex<SingleDescriptorMap>(rootIndex);

		if (rootIndexLocation)
			m_rootDescriptors[rootIndexLocation.value()].bufferAddress = bufferAddress;
		else
			m_rootDescriptors.emplace_back(
				SingleDescriptorMap{
					.rootIndex        = rootIndex,
					.bufferAddress    = bufferAddress,
					.bindViewFunction = BindViewFunction
				}
			);

		return *this;
	}

	template<void(*BindTableFunction)(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)>
	D3DDescriptorMap& SetDescriptorTable(UINT rootIndex, UINT descriptorIndex) noexcept
	{
		std::optional<size_t> rootIndexLocation = FindRootIndex<DescriptorTableMap>(rootIndex);

		if (rootIndexLocation)
			m_descriptorTables[rootIndexLocation.value()].descriptorIndex = descriptorIndex;
		else
			m_descriptorTables.emplace_back(
				DescriptorTableMap{
					.rootIndex         = rootIndex,
					.descriptorIndex   = descriptorIndex,
					.bindTableFunction = BindTableFunction
				}
			);

		return *this;
	}

private:
	std::vector<SingleDescriptorMap> m_rootDescriptors;
	std::vector<DescriptorTableMap>  m_descriptorTables;

public:
	D3DDescriptorMap(const D3DDescriptorMap& other) noexcept
		: m_rootDescriptors{ other.m_rootDescriptors },
		m_descriptorTables{ other.m_descriptorTables }
	{}
	D3DDescriptorMap& operator=(const D3DDescriptorMap& other) noexcept
	{
		m_rootDescriptors  = other.m_rootDescriptors;
		m_descriptorTables = other.m_descriptorTables;

		return *this;
	}

	D3DDescriptorMap(D3DDescriptorMap&& other) noexcept
		: m_rootDescriptors{ std::move(other.m_rootDescriptors) },
		m_descriptorTables{ std::move(other.m_descriptorTables) }
	{}
	D3DDescriptorMap& operator=(D3DDescriptorMap&& other) noexcept
	{
		m_rootDescriptors  = std::move(other.m_rootDescriptors);
		m_descriptorTables = std::move(other.m_descriptorTables);

		return *this;
	}
};

class D3DDescriptorManager
{
public:
	D3DDescriptorManager(ID3D12Device* device, size_t layoutCount);

	D3DDescriptorManager& AddConstants(
		size_t registerSlot, size_t registerSpace, UINT uintCount,
		D3D12_SHADER_VISIBILITY shaderStage
	) noexcept {
		m_descriptorLayouts[registerSpace].AddConstants(registerSlot, uintCount, shaderStage);

		return *this;
	}
	D3DDescriptorManager& AddCBVTable(
		size_t registerSlot, size_t registerSpace, UINT descriptorCount,
		D3D12_SHADER_VISIBILITY shaderStage, bool bindless
	) noexcept {
		m_descriptorLayouts[registerSpace].AddCBVTable(
			registerSlot, descriptorCount, shaderStage, bindless
		);

		return *this;
	}
	D3DDescriptorManager& AddSRVTable(
		size_t registerSlot, size_t registerSpace, UINT descriptorCount,
		D3D12_SHADER_VISIBILITY shaderStage, bool bindless
	) noexcept {
		m_descriptorLayouts[registerSpace].AddSRVTable(
			registerSlot, descriptorCount, shaderStage, bindless
		);

		return *this;
	}
	D3DDescriptorManager& AddUAVTable(
		size_t registerSlot, size_t registerSpace, UINT descriptorCount,
		D3D12_SHADER_VISIBILITY shaderStage, bool bindless
	) noexcept {
		m_descriptorLayouts[registerSpace].AddUAVTable(
			registerSlot, descriptorCount, shaderStage, bindless
		);

		return *this;
	}
	D3DDescriptorManager& AddRootCBV(
		size_t registerSlot, size_t registerSpace, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept {
		m_descriptorLayouts[registerSpace].AddRootCBV(registerSlot, shaderStage);

		return *this;
	}
	D3DDescriptorManager& AddRootSRV(
		size_t registerSlot, size_t registerSpace, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept {
		m_descriptorLayouts[registerSpace].AddRootSRV(registerSlot, shaderStage);

		return *this;
	}
	D3DDescriptorManager& AddRootUAV(
		size_t registerSlot, size_t registerSpace, D3D12_SHADER_VISIBILITY shaderStage
	) noexcept {
		m_descriptorLayouts[registerSpace].AddRootUAV(registerSlot, shaderStage);

		return *this;
	}

	void CreateDescriptors();
	void RecreateDescriptors(const std::vector<D3DDescriptorLayout>& oldLayouts);

	void BindDescriptorHeap(ID3D12GraphicsCommandList* commandList) const noexcept;
	void BindDescriptorHeap(const D3DCommandList& commandList) const noexcept
	{
		BindDescriptorHeap(commandList.Get());
	}
	void BindDescriptors(ID3D12GraphicsCommandList* commandList) const noexcept;
	void BindDescriptors(const D3DCommandList& commandList) const noexcept
	{
		BindDescriptors(commandList.Get());
	}

	void CreateCBV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex,
		const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc
	) const;
	void CreateSRV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex,
		ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc
	) const;
	void CreateUAV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex,
		ID3D12Resource* resource, ID3D12Resource* counterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc
	) const;

	void SetDescriptorCBV(
		D3D12_CPU_DESCRIPTOR_HANDLE handle,
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex
	) const;
	void SetDescriptorSRV(
		D3D12_CPU_DESCRIPTOR_HANDLE handle,
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex
	) const;
	void SetDescriptorUAV(
		D3D12_CPU_DESCRIPTOR_HANDLE handle,
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex
	) const;

	void SetRootCBV(
		size_t registerSlot, size_t registerSpace, D3D12_GPU_VIRTUAL_ADDRESS resourceAddress,
		bool graphicsQueue
	);
	void SetRootSRV(
		size_t registerSlot, size_t registerSpace, D3D12_GPU_VIRTUAL_ADDRESS resourceAddress,
		bool graphicsQueue
	);
	void SetRootUAV(
		size_t registerSlot, size_t registerSpace, D3D12_GPU_VIRTUAL_ADDRESS resourceAddress,
		bool graphicsQueue
	);
	void SetDescriptorTableCBV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex, bool graphicsQueue
	);
	void SetDescriptorTableSRV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex, bool graphicsQueue
	);
	void SetDescriptorTableUAV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex, bool graphicsQueue
	);

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleCBV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex
	) const noexcept;
	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleSRV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex
	) const noexcept;
	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleUAV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex
	) const noexcept;
	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleCBV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex
	) const noexcept;
	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleSRV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex
	) const noexcept;
	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleUAV(
		size_t registerSlot, size_t registerSpace, UINT descriptorIndex
	) const noexcept;

	[[nodiscard]]
	const std::vector<D3DDescriptorLayout>& GetLayouts() const noexcept { return m_descriptorLayouts; }

	[[nodiscard]]
	UINT GetRootIndexCBV(size_t registerIndex, size_t layoutIndex) const noexcept
	{
		return GetRootIndex<D3D12_DESCRIPTOR_RANGE_TYPE_CBV>(registerIndex, layoutIndex);
	}
	[[nodiscard]]
	UINT GetRootIndexSRV(size_t registerIndex, size_t layoutIndex) const noexcept
	{
		return GetRootIndex<D3D12_DESCRIPTOR_RANGE_TYPE_SRV>(registerIndex, layoutIndex);
	}
	[[nodiscard]]
	UINT GetRootIndexUAV(size_t registerIndex, size_t layoutIndex) const noexcept
	{
		return GetRootIndex<D3D12_DESCRIPTOR_RANGE_TYPE_UAV>(registerIndex, layoutIndex);
	}

private:
	[[nodiscard]]
	UINT GetLayoutOffset(size_t layoutIndex) const noexcept;
	[[nodiscard]]
	UINT GetDescriptorOffsetCBV(size_t registerIndex, size_t layoutIndex) const noexcept;
	[[nodiscard]]
	UINT GetDescriptorOffsetSRV(size_t registerIndex, size_t layoutIndex) const noexcept;
	[[nodiscard]]
	UINT GetDescriptorOffsetUAV(size_t registerIndex, size_t layoutIndex) const noexcept;
	[[nodiscard]]
	UINT GetDescriptorOffsetCBV(
		size_t registerIndex, size_t layoutIndex, UINT descriptorIndex
	) const noexcept;
	[[nodiscard]]
	UINT GetDescriptorOffsetSRV(
		size_t registerIndex, size_t layoutIndex, UINT descriptorIndex
	) const noexcept;
	[[nodiscard]]
	UINT GetDescriptorOffsetUAV(
		size_t registerIndex, size_t layoutIndex, UINT descriptorIndex
	) const noexcept;

	template<D3D12_DESCRIPTOR_RANGE_TYPE type>
	[[nodiscard]]
	UINT GetRootIndex(size_t registerIndex, size_t layoutIndex) const noexcept
	{
		// Each layout will be ordered one after another.
		const UINT localRootIndex = m_descriptorLayouts[layoutIndex].GetBindingIndex<type>(registerIndex);

		UINT rootOffset = 0u;

		for (size_t index = 0u; index < layoutIndex; ++index)
			rootOffset += static_cast<UINT>(m_descriptorLayouts[index].GetBindingCount());

		return localRootIndex + rootOffset;
	}

private:
	D3DDescriptorHeap                m_resourceHeapGPU;
	D3DDescriptorMap                 m_descriptorMap;
	D3DDescriptorHeap                m_resourceHeapCPU;
	ID3D12Device*                    m_device;
	std::vector<D3DDescriptorLayout> m_descriptorLayouts;

public:
	D3DDescriptorManager(const D3DDescriptorManager&) = delete;
	D3DDescriptorManager& operator=(const D3DDescriptorManager&) = delete;

	D3DDescriptorManager(D3DDescriptorManager&& other) noexcept
		: m_resourceHeapGPU{ std::move(other.m_resourceHeapGPU) },
		m_descriptorMap{ std::move(other.m_descriptorMap) },
		m_resourceHeapCPU{ std::move(other.m_resourceHeapCPU) },
		m_device{ other.m_device },
		m_descriptorLayouts{ std::move(other.m_descriptorLayouts) }
	{}
	D3DDescriptorManager& operator=(D3DDescriptorManager&& other) noexcept
	{
		m_resourceHeapGPU   = std::move(other.m_resourceHeapGPU);
		m_descriptorMap     = std::move(other.m_descriptorMap);
		m_resourceHeapCPU   = std::move(other.m_resourceHeapCPU);
		m_device            = other.m_device;
		m_descriptorLayouts = std::move(other.m_descriptorLayouts);

		return *this;
	}
};
#endif
