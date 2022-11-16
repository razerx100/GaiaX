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
		m_descriptorOffset{ 0u }, m_texture{ false } {}

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
		m_resourceBuffer.SetTextureInfo(width, height, format, msaa);
		m_resourceBuffer.ReserveHeapSpace(device);

		m_texture = true;
	}

	void SetBufferInfo(
		ID3D12Device* device,
		UINT strideSize, UINT elementsPerAllocation, size_t subAllocationCount
	) noexcept {
		UINT64 bufferSize = 0u;

		if (m_uav)
			bufferSize = ConfigureUAVInfo(
				strideSize, elementsPerAllocation, subAllocationCount
			);
		else
			bufferSize = ConfigureSRVInfo(
				strideSize, elementsPerAllocation, subAllocationCount
			);

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
		assert(m_uav && "Not a UAV.");
		return m_counterOffsets[index];
	}

	[[nodiscard]]
	UINT64 GetBufferOffset(size_t index) const noexcept {
		return m_bufferOffsets[index];
	}

	[[nodiscard]]
	std::uint8_t* GetBufferCPUWPointer(size_t index) const noexcept {
		return m_resourceBuffer.GetCPUWPointer() + GetBufferOffset(index);
	}

	[[nodiscard]]
	std::uint8_t* GetCounterCPUWPointer(size_t index) const noexcept {
		assert(m_uav && "Not a UAV.");
		return m_resourceBuffer.GetCPUWPointer() + GetCounterOffset(index);
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

	[[nodiscard]]
	UINT64 ConfigureUAVInfo(
		UINT strideSize, UINT elementCount, size_t subAllocationCount
	) noexcept {
		D3D12_BUFFER_UAV bufferInfo{};
		bufferInfo.StructureByteStride = strideSize;
		bufferInfo.NumElements = elementCount;

		const size_t bufferSubAllocationSize = static_cast<size_t>(strideSize) * elementCount;
		UINT64 bufferSize = 0u;

		if (D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT > strideSize) {
			// If stride is smaller, placing the counter at the start of each allocation
			// is more efficient

			UINT64 counterOffset = 0u;
			UINT64 firstElement = 1u;
			size_t bufferOffset = strideSize;
			// Counter is placed inside the first element of the resource and the actual buffer
			// starts from the second index

			for (size_t index = 0u; index < subAllocationCount; ++index) {
				bufferInfo.CounterOffsetInBytes = counterOffset;
				bufferInfo.FirstElement = firstElement;

				m_bufferInfos.emplace_back(bufferInfo);
				m_counterOffsets.emplace_back(counterOffset);
				m_bufferOffsets.emplace_back(static_cast<UINT64>(bufferOffset));

				const size_t allocationSize = bufferOffset + bufferSubAllocationSize;

				bufferSize = static_cast<UINT64>(allocationSize);
				counterOffset = static_cast<UINT64>(Align(
					allocationSize, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT
				)); // Align to find the next counter position
				bufferOffset = Align(counterOffset, strideSize); // Next buffer starts after
				firstElement = bufferOffset / strideSize;        // one element
			}
		}
		else {
			// If stride is larger, placing the counter at the end of each allocation
			// is more efficient

			UINT64 counterOffset = 0u;
			UINT64 firstElement = 0u;
			size_t bufferOffset = 0u;
			// The actual buffer is placed at first and then the end of the buffer is aligned
			// to place the counter

			for (size_t index = 0u; index < subAllocationCount; ++index) {
				bufferInfo.FirstElement = firstElement;

				m_bufferOffsets.emplace_back(static_cast<UINT64>(bufferOffset));

				const size_t allocationSize = bufferOffset + bufferSubAllocationSize;
				counterOffset = static_cast<UINT64>(Align(
					allocationSize, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT
				)); // Align to find the current counter position

				bufferInfo.CounterOffsetInBytes = counterOffset;

				m_bufferInfos.emplace_back(bufferInfo);
				m_counterOffsets.emplace_back(counterOffset);

				bufferOffset = Align(counterOffset, strideSize); // Next buffer starts after
				firstElement = bufferOffset / strideSize;        // one element
			}

			bufferSize = static_cast<UINT64>(bufferOffset);
		}

		return bufferSize;
	}

	[[nodiscard]]
	UINT64 ConfigureSRVInfo(
		UINT strideSize, UINT elementCount, size_t subAllocationCount
	) noexcept {
		D3D12_BUFFER_UAV bufferInfo{};
		bufferInfo.StructureByteStride = strideSize;
		bufferInfo.NumElements = elementCount;

		auto subAllocationSize = static_cast<UINT64>(strideSize * elementCount);

		for (size_t index = 0u; index < subAllocationCount; ++index) {
			bufferInfo.FirstElement = index * elementCount;

			m_bufferInfos.emplace_back(bufferInfo);
			m_bufferOffsets.emplace_back(static_cast<UINT64>(index * subAllocationSize));
		}

		return static_cast<UINT64>(subAllocationSize * subAllocationCount);
	}

protected:
	ResourceView m_resourceBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandleStart;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandleStart;
	size_t m_descriptorSize;
	bool m_uav;
	size_t m_descriptorOffset;
	bool m_texture;
	std::vector<D3D12_BUFFER_UAV> m_bufferInfos;
	std::vector<UINT64> m_counterOffsets;
	std::vector<UINT64> m_bufferOffsets;
};

template<>
void D3DDescriptorView<D3DUploadableResourceView>::SetTextureInfo(
	ID3D12Device* device, UINT64 width, UINT height, DXGI_FORMAT format, bool msaa
);

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
