#include <DescriptorTableManager.hpp>
#include <D3DThrowMacros.hpp>

DescriptorTableManager::DescriptorTableManager()
	: m_descriptorCount(0u), m_colorRangeStart(0u), m_textureRangeStart(0u) {}

void DescriptorTableManager::CreateDescriptorTable(ID3D12Device* device) {
	m_descriptorCount += m_sharedColorCPUHandle.size();

	m_uploadDescHeap = CreateDescHeap(device, m_descriptorCount, false);

	size_t descriptorSize = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
		m_uploadDescHeap->GetCPUDescriptorHandleForHeapStart();

	SetSharedAddresses(
		m_sharedColorCPUHandle, m_sharedColorIndices, cpuHandle.ptr,
		static_cast<SIZE_T>(descriptorSize),
		m_colorRangeStart
	);

	m_textureRangeStart = static_cast<size_t>(*m_sharedColorIndices.back()) + 1u;

	m_pDescHeap = CreateDescHeap(device, m_descriptorCount);
}

ResourceAddress DescriptorTableManager::GetColorIndex() noexcept {
	m_sharedColorIndices.emplace_back(std::make_shared<SharedAddress>());
	m_sharedColorCPUHandle.emplace_back(std::make_shared<_SharedAddress<SIZE_T>>());

	return {
		m_sharedColorIndices.back(),
		m_sharedColorCPUHandle.back()
	};
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTableManager::GetColorRangeStart() const noexcept {
	return D3D12_GPU_DESCRIPTOR_HANDLE{ static_cast<UINT64>(m_colorRangeStart) };
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTableManager::GetTextureRangeStart() const noexcept {
	return D3D12_GPU_DESCRIPTOR_HANDLE{ static_cast<UINT64>(m_textureRangeStart) };
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
	m_sharedColorCPUHandle = std::vector<SharedCPUHandle>();
	m_uploadDescHeap.Reset();
}

void DescriptorTableManager::SetSharedAddresses(
	std::vector<SharedCPUHandle>& sharedHandles,
	std::vector<SharedIndex>& sharedIndices,
	SIZE_T& cpuHandle, SIZE_T descIncSize,
	size_t indicesStart
) const noexcept {
	for (size_t index = 0u; index < sharedHandles.size(); ++index) {
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
