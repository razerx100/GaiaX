#ifndef D3D_DESCRIPTOR_LAYOUT_HPP_
#define D3D_DESCRIPTOR_LAYOUT_HPP_
#include <D3DHeaders.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <vector>

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
#endif
