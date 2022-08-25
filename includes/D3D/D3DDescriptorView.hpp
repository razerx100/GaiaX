#ifndef D3D_DESCRIPTOR_VIEW_HPP_
#define D3D_DESCRIPTOR_VIEW_HPP_
#include <AddressContainer.hpp>
#include <D3DResource.hpp>
#include <D3DHelperFunctions.hpp>
#include <vector>

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

template<class ResourceView>
class D3DDescriptorView {
public:
	D3DDescriptorView(
		ResourceType type,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	)
		: m_resourceBuffer{ type, flags }, m_gpuHandleStart{}, m_cpuHandleStart{},
		m_descriptorSize{ 0u },
		m_uav{ flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS },
		m_subAllocationSize{ 0u }, m_strideSize{ 0u }, m_descriptorOffset{ 0u },
		m_texture{ false } {}

	void SetDescriptorOffset(
		size_t descriptorOffset, size_t descriptorSize
	) noexcept {
		m_descriptorOffset = descriptorOffset;
		m_descriptorSize = descriptorSize;
	}

	void UpdateDescriptorOffset(size_t descriptorOffset) noexcept {
		m_descriptorOffset += descriptorOffset;
	}

	void SetTextureInfo(
		ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
	) {
		m_resourceBuffer.SetTextureInfo(device, width, height, format, msaa);
		m_resourceBuffer.ReserveHeapSpace(device);

		m_texture = true;
	}

	void SetBufferInfo(
		ID3D12Device* device,
		UINT strideSize, UINT elementsPerAllocation, size_t subAllocationCount
	) noexcept {
		size_t bufferSize = 0u;
		m_strideSize = strideSize;

		D3D12_BUFFER_UAV bufferInfo{};
		bufferInfo.StructureByteStride = strideSize;
		bufferInfo.NumElements = elementsPerAllocation;

		size_t bufferSizePerAllocation =
			static_cast<size_t>(strideSize) * elementsPerAllocation;

		if (m_uav) {
			UINT64 counterOffset = 0u;
			UINT64 firstElement = 1u;
			UINT64 alignedSubAllocationSize = Align(
				strideSize + bufferSizePerAllocation, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT
			);

			for (size_t index = 0u; index < subAllocationCount; ++index) {
				bufferInfo.CounterOffsetInBytes = counterOffset;
				bufferInfo.FirstElement = firstElement;

				m_bufferInfos.emplace_back(bufferInfo);

				bufferSize += alignedSubAllocationSize + sizeof(UINT);

				counterOffset += alignedSubAllocationSize;
				firstElement = Align(counterOffset + sizeof(UINT), strideSize) / strideSize;
			}

			m_subAllocationSize = alignedSubAllocationSize;
		}
		else {
			for (size_t index = 0u; index < subAllocationCount; ++index) {
				bufferInfo.FirstElement = index * elementsPerAllocation;

				m_bufferInfos.emplace_back(bufferInfo);

				bufferSize += bufferSizePerAllocation;
			}

			m_subAllocationSize = bufferSizePerAllocation;
		}

		m_resourceBuffer.SetBufferInfo(bufferSize);
		m_resourceBuffer.ReserveHeapSpace(device);
	}

	void CreateDescriptorView(
		ID3D12Device* device,
		D3D12_CPU_DESCRIPTOR_HANDLE uploadDescriptorStart,
		D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorStart,
		D3D12_RESOURCE_STATES initialState
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
	UINT64 GetCounterOffset(size_t index) const noexcept {
		return static_cast<UINT64>(index * m_subAllocationSize);
	}

	[[nodiscard]]
	std::uint8_t* GetCPUWPointer(size_t index) const noexcept {
		return m_resourceBuffer.GetCPUWPointer()
			+ index * m_subAllocationSize + m_uav * m_strideSize;
	}

	[[nodiscard]]
	ID3D12Resource* GetResource() const noexcept {
		return m_resourceBuffer.GetResource();
	}

	[[nodiscard]]
	D3D12_RESOURCE_DESC GetResourceDesc() const noexcept {
		return m_resourceBuffer.GetResourceDesc();
	}

private:
	void CreateBufferDescriptors(ID3D12Device* device) {
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_cpuHandleStart;

		if (m_uav) {
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

			for (auto& bufferInfo : m_bufferInfos) {
				desc.Buffer = bufferInfo;

				device->CreateUnorderedAccessView(
					m_resourceBuffer.GetResource(), m_resourceBuffer.GetResource(),
					&desc, cpuHandle
				);

				cpuHandle.ptr += m_descriptorSize;
			}
		}
		else {
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			for (auto& bufferInfo : m_bufferInfos) {
				desc.Buffer.FirstElement = bufferInfo.FirstElement;
				desc.Buffer.NumElements = bufferInfo.NumElements;
				desc.Buffer.StructureByteStride = bufferInfo.StructureByteStride;

				device->CreateShaderResourceView(
					m_resourceBuffer.GetResource(), &desc, cpuHandle
				);

				cpuHandle.ptr += m_descriptorSize;
			}
		}
	}

	void CreateTextureDescriptors(ID3D12Device* device) {
		const D3D12_RESOURCE_DESC textureDesc = m_resourceBuffer.GetResourceDesc();

		if (m_uav) {
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			desc.Format = textureDesc.Format;

			device->CreateUnorderedAccessView(
				m_resourceBuffer.GetResource(), m_resourceBuffer.GetResource(),
				&desc, m_cpuHandleStart
			);
		}
		else {
			D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Format = textureDesc.Format;
			desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			desc.Texture2D.MipLevels = textureDesc.MipLevels;

			device->CreateShaderResourceView(
				m_resourceBuffer.GetResource(), &desc, m_cpuHandleStart
			);
		}
	}

protected:
	ResourceView m_resourceBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandleStart;
	size_t m_descriptorSize;
	bool m_uav;
	size_t m_subAllocationSize;
	size_t m_strideSize;
	size_t m_descriptorOffset;
	bool m_texture;
	std::vector<D3D12_BUFFER_UAV> m_bufferInfos;
};

using D3DSingleDescriptorView = D3DDescriptorView<D3DResourceView>;

class D3DUploadResourceDescriptorView : public D3DDescriptorView<D3DUploadableResourceView> {
public:
	D3DUploadResourceDescriptorView(
		ResourceType type = ResourceType::gpuOnly,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	) noexcept : D3DDescriptorView<D3DUploadableResourceView>{ type, flags } {}

	void RecordResourceUpload(ID3D12GraphicsCommandList* copyList) noexcept;
	void ReleaseUploadResource() noexcept;

	[[nodiscard]]
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const noexcept;
};
#endif
