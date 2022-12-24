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
class _D3DDescriptorViewBase {
public:
	_D3DDescriptorViewBase(ResourceType type, D3D12_RESOURCE_FLAGS flags) noexcept
		: m_resourceBuffer{ type, flags }, m_gpuHandleStart{}, m_cpuHandleStart{},
		m_descriptorSize{ 0u }, m_descriptorOffset{ 0u } {}

	virtual ~_D3DDescriptorViewBase() = default;

	void SetDescriptorOffset(size_t descriptorOffset, size_t descriptorSize) noexcept {
		m_descriptorOffset = descriptorOffset;
		m_descriptorSize = descriptorSize;
	}

	void UpdateDescriptorOffset(size_t descriptorOffset) noexcept {
		m_descriptorOffset += descriptorOffset;
	}

	[[nodiscard]]
	ID3D12Resource* GetResource() const noexcept {
		return m_resourceBuffer.GetResource();
	}
	[[nodiscard]]
	D3D12_RESOURCE_DESC GetResourceDesc() const noexcept {
		return m_resourceBuffer.GetResourceDesc();
	}

protected:
	ResourceView m_resourceBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandleStart;
	size_t m_descriptorSize;
	size_t m_descriptorOffset;
};

template<class ResourceView>
class _D3DDescriptorView : public _D3DDescriptorViewBase<ResourceView> {
public:
	_D3DDescriptorView(ResourceType type, DescriptorType descType) noexcept
		: _D3DDescriptorViewBase<ResourceView>(
			type, DescriptorType::SRV == descType ?
			D3D12_RESOURCE_FLAG_NONE : D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			), m_descType{descType}, m_texture{false}, m_strideSize{ 0u },
		m_elementCount{ 0u }, m_subAllocationCount{ 0u } {}

	void SetTextureInfo(
		ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
	) noexcept {
		this->m_resourceBuffer.SetTextureInfo(width, height, format, msaa);
		this->m_resourceBuffer.ReserveHeapSpace(device);

		this->m_texture = true;
	}

	void SetBufferInfo(
		ID3D12Device* device,
		UINT strideSize, UINT elementsPerAllocation, size_t subAllocationCount = 1u
	) noexcept {
		m_strideSize = strideSize;
		m_elementCount = elementsPerAllocation;
		m_subAllocationCount = static_cast<UINT64>(subAllocationCount);

		this->m_resourceBuffer.SetBufferInfo(
			strideSize * elementsPerAllocation, subAllocationCount
		);
		this->m_resourceBuffer.ReserveHeapSpace(device);
	}

	void CreateDescriptorView(
		ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart,
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart, D3D12_RESOURCE_STATES initialState
	) noexcept {
		this->m_cpuHandleStart.ptr =
		{ uploadDescriptorStart.ptr + this->m_descriptorSize * this->m_descriptorOffset };
		this->m_gpuHandleStart.ptr =
		{ gpuDescriptorStart.ptr + this->m_descriptorSize * this->m_descriptorOffset };

		this->m_resourceBuffer.CreateResource(device, initialState);

		if (m_texture)
			CreateTextureDescriptors(device);
		else
			CreateBufferDescriptors(device);
	}

	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(size_t index) const noexcept {
		return D3D12_GPU_DESCRIPTOR_HANDLE{
			this->m_gpuHandleStart.ptr + (this->m_descriptorSize * index)
		};
	}
	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetFirstGPUDescriptorHandle() const noexcept {
		return GetGPUDescriptorHandle(0u);
	}
	[[nodiscard]]
	std::uint8_t* GetCPUWPointer(UINT64 index) const noexcept {
		return this->m_resourceBuffer.GetCPUWPointer(index);
	}
	[[nodiscard]]
	std::uint8_t* GetFirstCPUWPointer() const noexcept {
		return this->m_resourceBuffer.GetFirstCPUWPointer();
	}
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(UINT64 index) const noexcept {
		return this->m_resourceBuffer.GetGPUAddress(index);
	}
	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetFirstGPUAddress() const noexcept {
		return this->m_resourceBuffer.GetFirstGPUAddress();
	}
	[[nodiscard]]
	UINT64 GetSubAllocationOffset(UINT64 index) const noexcept {
		return this->m_resourceBuffer.GetSubAllocationOffset(index);
	}
	[[nodiscard]]
	UINT64 GetFirstSubAllocationOffset() const noexcept {
		return this->m_resourceBuffer.GetFirstSubAllocationOffset();
	}

private:
	void CreateBufferDescriptors(ID3D12Device* device) const noexcept {
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = this->m_cpuHandleStart;

		if (DescriptorType::UAV == m_descType) {
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{
				.Format = DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION_BUFFER
			};

			auto bufferInfos = GetBufferInfoUAV();
			for (auto& bufferInfo : bufferInfos) {
				desc.Buffer = bufferInfo;

				device->CreateUnorderedAccessView(
					this->m_resourceBuffer.GetResource(), this->m_resourceBuffer.GetResource(),
					&desc, cpuHandle
				);

				cpuHandle.ptr += this->m_descriptorSize;
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
					this->m_resourceBuffer.GetResource(), &desc, cpuHandle
				);

				cpuHandle.ptr += this->m_descriptorSize;
			}
		}
	}

	void CreateTextureDescriptors(ID3D12Device* device) const noexcept {
		const D3D12_RESOURCE_DESC textureDesc = this->m_resourceBuffer.GetResourceDesc();

		D3D12_SHADER_RESOURCE_VIEW_DESC desc{
		.Format = textureDesc.Format,
		.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
		};
		desc.Texture2D.MipLevels = textureDesc.MipLevels;

		device->CreateShaderResourceView(
			this->m_resourceBuffer.GetResource(), &desc, this->m_cpuHandleStart
		);
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
	bool m_texture;
	DescriptorType m_descType;
	UINT m_strideSize;
	UINT m_elementCount;
	UINT64 m_subAllocationCount;
};

template<>
void _D3DDescriptorView<D3DUploadableResourceView>::SetTextureInfo(
	ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
) noexcept;

using D3DDescriptorView = _D3DDescriptorView<D3DResourceView>;

class D3DUploadResourceDescriptorView : public _D3DDescriptorView<D3DUploadableResourceView> {
public:
	D3DUploadResourceDescriptorView(
		DescriptorType descType, ResourceType resType = ResourceType::gpuOnly
	) noexcept : _D3DDescriptorView<D3DUploadableResourceView>{ resType, descType } {}

	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;
};

class D3DDescriptorViewUAVCounter : public _D3DDescriptorViewBase<D3DResourceView> {
public:
	D3DDescriptorViewUAVCounter(ResourceType type) noexcept;

	void SetBufferInfo(ID3D12Device* device, UINT strideSize, UINT elementCount) noexcept;
	void CreateDescriptorView(
		ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart,
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart
	) noexcept;

	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const noexcept;
	[[nodiscard]]
	UINT64 GetCounterOffset() const noexcept;
	[[nodiscard]]
	UINT64 GetBufferOffset() const noexcept;

private:
	void CreateBufferDescriptors(ID3D12Device* device) const noexcept;

private:
	UINT64 m_counterOffset;
	UINT m_strideSize;
	UINT m_elementCount;
};
#endif
