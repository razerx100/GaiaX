#ifndef D3D_DESCRIPTOR_VIEW_HPP_
#define D3D_DESCRIPTOR_VIEW_HPP_
#include <AddressContainer.hpp>
#include <D3DResource.hpp>
#include <D3DHelperFunctions.hpp>
#include <vector>
#include <type_traits>
#include <cassert>

class D3DRootDescriptorView {
public:
	void SetAddressesStart(
		size_t addressesStart, size_t subAllocationSize, size_t alignment
	) noexcept;

	void UpdateCPUAddressStart(std::uint8_t* offset) noexcept;
	void UpdateGPUAddressStart(D3D12_GPU_VIRTUAL_ADDRESS offset) noexcept;

	[[nodiscard]]
	std::uint8_t* GetCPUAddressStart(size_t index) const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddressStart(size_t index) const noexcept;

private:
	SingleAddressContainer m_cpuAddress;
	SingleAddressContainer m_gpuAddress;
};

enum class DescriptorType {
	SRV,
	UAV,
	None
};

template<class ResourceView>
class _D3DDescriptorView {
public:
	_D3DDescriptorView(ResourceType type, DescriptorType descType)
		: m_resourceBuffer{
			type, DescriptorType::SRV == descType ?
			D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		}, m_gpuHandleStart{}, m_cpuHandleStart{}, m_descriptorSize{ 0u },
		m_descriptorOffset{ 0u }, m_texture{ false }, m_descType{ descType },
		m_strideSize{ 0u }, m_elementCount{ 0u }, m_subAllocationCount{ 0u } {}

	void SetDescriptorOffset(size_t descriptorOffset, size_t descriptorSize) noexcept {
		m_descriptorOffset = descriptorOffset;
		m_descriptorSize = descriptorSize;
	}

	void UpdateDescriptorOffset(size_t descriptorOffset) noexcept {
		m_descriptorOffset += descriptorOffset;
	}

	void SetTextureInfo(
		ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
	) {
		m_resourceBuffer.SetTextureInfo(width, height, format, msaa);
		m_resourceBuffer.ReserveHeapSpace(device);

		m_texture = true;
	}

	void SetBufferInfo(
		ID3D12Device* device,
		UINT strideSize, UINT elementsPerAllocation, size_t subAllocationCount = 1u
	) noexcept {
		m_strideSize = strideSize;
		m_elementCount = elementsPerAllocation;
		m_subAllocationCount = static_cast<UINT64>(subAllocationCount);

		m_resourceBuffer.SetBufferInfo(strideSize * elementsPerAllocation, subAllocationCount);
		m_resourceBuffer.ReserveHeapSpace(device);
	}

	void CreateDescriptorView(
		ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart,
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart, D3D12_RESOURCE_STATES initialState
	) {
		m_cpuHandleStart.ptr =
		{ uploadDescriptorStart.ptr + m_descriptorSize * m_descriptorOffset };
		m_gpuHandleStart.ptr =
		{ gpuDescriptorStart.ptr + m_descriptorSize * m_descriptorOffset };

		m_resourceBuffer.CreateResource(device, initialState);

		if (m_texture)
			CreateTextureDescriptors(device);
		else
			CreateBufferDescriptors(device);
	}

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(size_t index) const noexcept {
		return D3D12_CPU_DESCRIPTOR_HANDLE{ m_cpuHandleStart.ptr + (m_descriptorSize * index) };
	}
	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(size_t index) const noexcept {
		return D3D12_GPU_DESCRIPTOR_HANDLE{ m_gpuHandleStart.ptr + (m_descriptorSize * index) };
	}
	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetFirstCPUDescriptorHandle() const noexcept {
		return GetCPUDescriptorHandle(0u);
	}
	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetFirstGPUDescriptorHandle() const noexcept {
		return GetGPUDescriptorHandle(0u);
	}
	[[nodiscard]]
	ID3D12Resource* GetResource() const noexcept {
		return m_resourceBuffer.GetResource();
	}
	[[nodiscard]]
	D3D12_RESOURCE_DESC GetResourceDesc() const noexcept {
		return m_resourceBuffer.GetResourceDesc();
	}
	[[nodiscard]]
	std::uint8_t* GetCPUWPointer(UINT64 index) const noexcept {
		return m_resourceBuffer.GetCPUWPointer(index);
	}
	[[nodiscard]]
	std::uint8_t* GetFirstCPUWPointer() const noexcept {
		return m_resourceBuffer.GetFirstCPUWPointer();
	}
	[[nodiscard]]
	UINT64 GetSubAllocationOffset(UINT64 index) const noexcept {
		return m_resourceBuffer.GetSubAllocationOffset(index);
	}
	[[nodiscard]]
	UINT64 GetFirstSubAllocationOffset() const noexcept {
		return m_resourceBuffer.GetFirstSubAllocationOffset();
	}

private:
	void CreateBufferDescriptors(ID3D12Device* device) {
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_cpuHandleStart;

		if (DescriptorType::UAV == m_descType) {
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{
				.Format = DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION_BUFFER
			};

			auto bufferInfos = GetBufferInfoUAV();
			for (auto& bufferInfo : bufferInfos) {
				desc.Buffer = bufferInfo;

				device->CreateUnorderedAccessView(
					m_resourceBuffer.GetResource(), m_resourceBuffer.GetResource(),
					&desc, cpuHandle
				);

				cpuHandle.ptr += m_descriptorSize;
			}
		}
		else {
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{
				.Format = DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
			};

			auto bufferInfos = GetBufferInfoSRV();
			for (auto& bufferInfo : bufferInfos) {
				desc.Buffer = bufferInfo;

				device->CreateShaderResourceView(
					m_resourceBuffer.GetResource(), &desc, cpuHandle
				);

				cpuHandle.ptr += m_descriptorSize;
			}
		}
	}

	void CreateTextureDescriptors(ID3D12Device* device) {
		const D3D12_RESOURCE_DESC textureDesc = m_resourceBuffer.GetResourceDesc();

		if (DescriptorType::UAV == m_descType) {
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{
				.Format = textureDesc.Format,
				.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D
			};

			device->CreateUnorderedAccessView(
				m_resourceBuffer.GetResource(), m_resourceBuffer.GetResource(),
				&desc, m_cpuHandleStart
			);
		}
		else {
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{
				.Format = textureDesc.Format,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
			};
			desc.Texture2D.MipLevels = textureDesc.MipLevels,

			device->CreateShaderResourceView(
				m_resourceBuffer.GetResource(), &desc, m_cpuHandleStart
			);
		}
	}

	[[nodiscard]]
	std::vector<D3D12_BUFFER_SRV> GetBufferInfoSRV() const noexcept {
		std::vector<D3D12_BUFFER_SRV> bufferInfos;
		for (UINT64 index = 0u; index < m_subAllocationCount; ++index) {
			D3D12_BUFFER_SRV bufferInfo{
				.FirstElement = index * m_elementCount,
				.NumElements = m_elementCount,
				.StructureByteStride = m_strideSize,
				.Flags = D3D12_BUFFER_SRV_FLAG_NONE
			};

			bufferInfos.emplace_back(bufferInfo);
		}

		return bufferInfos;
	}

	[[nodiscard]]
	std::vector<D3D12_BUFFER_UAV> GetBufferInfoUAV() const noexcept {
		std::vector<D3D12_BUFFER_UAV> bufferInfos;
		for (UINT64 index = 0u; index < m_subAllocationCount; ++index) {
			D3D12_BUFFER_UAV bufferInfo{
				.FirstElement = index * m_elementCount,
				.NumElements = m_elementCount,
				.StructureByteStride = m_strideSize,
				.CounterOffsetInBytes = 0u,
				.Flags = D3D12_BUFFER_UAV_FLAG_NONE
			};

			bufferInfos.emplace_back(bufferInfo);
		}

		return bufferInfos;
	}

protected:
	ResourceView m_resourceBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandleStart;
	size_t m_descriptorSize;
	size_t m_descriptorOffset;
	bool m_texture;
	DescriptorType m_descType;
	UINT m_strideSize;
	UINT m_elementCount;
	UINT64 m_subAllocationCount;
};

template<>
void _D3DDescriptorView<D3DUploadableResourceView>::SetTextureInfo(
	ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
);

using D3DDescriptorView = _D3DDescriptorView<D3DResourceView>;

class D3DUploadResourceDescriptorView : public _D3DDescriptorView<D3DUploadableResourceView> {
public:
	D3DUploadResourceDescriptorView(
		DescriptorType descType, ResourceType resType = ResourceType::gpuOnly
	) noexcept : _D3DDescriptorView<D3DUploadableResourceView>{ resType, descType } {}

	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(UINT64 index) const noexcept;
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetFirstGPUAddress() const noexcept;
};
#endif
