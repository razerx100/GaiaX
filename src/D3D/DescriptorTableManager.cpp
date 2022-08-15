#include <DescriptorTableManager.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>

DescriptorTableManager::DescriptorTableManager()
	: m_genericDescriptorCount{0u}, m_textureDescriptorCount{0u} {}

void DescriptorTableManager::CreateDescriptorTable(ID3D12Device* device) {
	size_t totalDescriptorCount = m_genericDescriptorCount + m_textureDescriptorCount;

	if (!totalDescriptorCount)
		++totalDescriptorCount;

	m_uploadDescHeap = CreateDescHeap(device, totalDescriptorCount, false);

	m_pDescHeap = CreateDescHeap(device, totalDescriptorCount);
}

size_t DescriptorTableManager::ReserveDescriptorsTextureAndGetRelativeOffset(
	size_t descriptorCount
) noexcept {
	m_textureDescriptorSet.emplace_back(descriptorCount);

	const size_t descriptorOffset = m_textureDescriptorCount;
	m_textureDescriptorCount += descriptorCount;

	return descriptorOffset;
}

size_t DescriptorTableManager::ReserveDescriptorsAndGetOffset(
	size_t descriptorCount
) noexcept {
	m_genericDescriptorSet.emplace_back(descriptorCount);

	const size_t descriptorOffset = m_genericDescriptorCount;
	m_genericDescriptorCount += descriptorCount;

	return descriptorOffset;
}

size_t DescriptorTableManager::GetTextureRangeStart() const noexcept {
	return m_genericDescriptorCount;
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
	m_uploadDescHeap.Reset();
}

void DescriptorTableManager::CopyUploadHeap(ID3D12Device* device) {
	size_t totalDescriptorCount = m_genericDescriptorCount + m_textureDescriptorCount;

	if (!totalDescriptorCount)
		++totalDescriptorCount;

	device->CopyDescriptorsSimple(
		static_cast<UINT>(totalDescriptorCount),
		m_pDescHeap->GetCPUDescriptorHandleForHeapStart(),
		m_uploadDescHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
}

size_t DescriptorTableManager::GetTextureDescriptorCount() const noexcept {
	return m_textureDescriptorCount;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorTableManager::GetUploadDescriptorStart() const noexcept {
	return m_uploadDescHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTableManager::GetGPUDescriptorStart() const noexcept {
	return m_pDescHeap->GetGPUDescriptorHandleForHeapStart();
}
