#include <DescriptorTableManager.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>

DescriptorTableManager::DescriptorTableManager()
	: m_descriptorCount(0u), m_textureRangeStart{} {}

void DescriptorTableManager::CreateDescriptorTable(ID3D12Device* device) {
	m_descriptorCount += std::size(m_genericCPUHandles);

	size_t textureRangeStart = m_descriptorCount;

	m_descriptorCount += std::size(m_sharedTextureCPUHandle);

	if (!m_descriptorCount)
		++m_descriptorCount;

	m_uploadDescHeap = CreateDescHeap(device, m_descriptorCount, false);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
		m_uploadDescHeap->GetCPUDescriptorHandleForHeapStart();

	m_pDescHeap = CreateDescHeap(device, m_descriptorCount);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_pDescHeap->GetGPUDescriptorHandleForHeapStart();

	const size_t descriptorSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	for (size_t index = 0u; index < std::size(m_genericCPUHandles); ++index) {
		*m_genericCPUHandles[index] = cpuHandle.ptr;
		*m_genericGPUHandles[index] = gpuHandle.ptr;

		cpuHandle.ptr += descriptorSize;
		gpuHandle.ptr += descriptorSize;
	}

	// Texture
	SetSharedAddresses(
		m_sharedTextureCPUHandle, m_sharedTextureIndices, cpuHandle.ptr,
		static_cast<SIZE_T>(descriptorSize), textureRangeStart
	);

	m_textureRangeStart = CD3DX12_GPU_DESCRIPTOR_HANDLE(
		gpuHandle,
		static_cast<UINT>(textureRangeStart), static_cast<UINT>(descriptorSize)
	);
}

ResourceAddress DescriptorTableManager::ReserveDescriptorTexture() noexcept {
	auto sharedTextureIndex = std::make_shared<SharedAddress>();
	auto sharedTextureCPUHandle = std::make_shared<_SharedAddress<SIZE_T>>();

	m_sharedTextureIndices.emplace_back(sharedTextureIndex);
	m_sharedTextureCPUHandle.emplace_back(sharedTextureCPUHandle);

	return { std::move(sharedTextureIndex), std::move(sharedTextureCPUHandle) };
}

SharedDescriptorHandles DescriptorTableManager::ReserveDescriptor() noexcept {
	auto sharedCPUHandle = std::make_shared<_SharedAddress<SIZE_T>>();
	auto sharedGPUHandle = std::make_shared<_SharedAddress<UINT64>>();

	m_genericCPUHandles.emplace_back(sharedCPUHandle);
	m_genericGPUHandles.emplace_back(sharedGPUHandle);

	return { std::move(sharedCPUHandle), std::move(sharedGPUHandle) };
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTableManager::GetTextureRangeStart() const noexcept {
	return m_textureRangeStart;
}

ID3D12DescriptorHeap* DescriptorTableManager::GetDescHeapRef() const noexcept {
	return m_pDescHeap.Get();
}

ComPtr<ID3D12DescriptorHeap> DescriptorTableManager::CreateDescHeap(
	ID3D12Device* device, size_t descriptorCount, bool shaderVisible
) const {
	D3D12_DESCRIPTOR_HEAP_DESC descDesc = {};
	descDesc.NumDescriptors = static_cast<UINT>(descriptorCount);
	descDesc.NodeMask = 0u;
	descDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	descDesc.Flags =
		shaderVisible ?
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ComPtr<ID3D12DescriptorHeap> descHeap;

	HRESULT hr;
	D3D_THROW_FAILED(hr,
		device->CreateDescriptorHeap(&descDesc, __uuidof(ID3D12DescriptorHeap), &descHeap)
	);

	return descHeap;
}

void DescriptorTableManager::ReleaseUploadHeap() noexcept {
	m_sharedTextureCPUHandle = std::vector<SharedCPUHandle>();
	m_uploadDescHeap.Reset();
}

void DescriptorTableManager::SetSharedAddresses(
	std::vector<SharedCPUHandle>& sharedHandles,
	std::vector<SharedIndex>& sharedIndices,
	SIZE_T& cpuHandle, SIZE_T descIncSize,
	size_t indicesStart
) const noexcept {
	for (size_t index = 0u; index < std::size(sharedHandles); ++index) {
		*sharedHandles[index] = cpuHandle;
		*sharedIndices[index] = indicesStart + index;

		cpuHandle += descIncSize;
	}
}

void DescriptorTableManager::CopyUploadHeap(ID3D12Device* device) {
	device->CopyDescriptorsSimple(
		static_cast<UINT>(m_descriptorCount),
		m_pDescHeap->GetCPUDescriptorHandleForHeapStart(),
		m_uploadDescHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
}

size_t DescriptorTableManager::GetTextureDescriptorCount() const noexcept {
	return std::size(m_sharedTextureCPUHandle);
}
