#include <DescriptorTableManager.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>

DescriptorTableManager::DescriptorTableManager()
	: m_descriptorCount(0u), m_textureRangeStart{} {}

void DescriptorTableManager::CreateDescriptorTable(ID3D12Device* device) {
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

		cpuHandle.ptr += descriptorSize * m_genericDescriptorCounts[index];
		gpuHandle.ptr += descriptorSize * m_genericDescriptorCounts[index];
	}

	// Texture
	m_textureRangeStart = gpuHandle;

	for (size_t index = 0u; index < std::size(m_sharedTextureCPUHandle); ++index) {
		*m_sharedTextureCPUHandle[index] = cpuHandle.ptr;

		cpuHandle.ptr += descriptorSize * m_textureDescriptorCounts[index];
	}
}

SharedCPUHandle DescriptorTableManager::ReserveDescriptorsTexture(
	size_t descriptorCount
) noexcept {
	auto sharedTextureCPUHandle = std::make_shared<ShareableCPUHandle>();

	m_sharedTextureCPUHandle.emplace_back(sharedTextureCPUHandle);
	m_textureDescriptorCounts.emplace_back(descriptorCount);

	m_descriptorCount += descriptorCount;

	return sharedTextureCPUHandle;
}

SharedDescriptorHandles DescriptorTableManager::ReserveDescriptors(
	size_t descriptorCount
) noexcept {
	auto sharedCPUHandle = std::make_shared<ShareableCPUHandle>();
	auto sharedGPUHandle = std::make_shared<ShareableGPUHandle>();

	m_genericCPUHandles.emplace_back(sharedCPUHandle);
	m_genericGPUHandles.emplace_back(sharedGPUHandle);
	m_genericDescriptorCounts.emplace_back(descriptorCount);

	m_descriptorCount += descriptorCount;

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
