#include <CPUAccessibleStorage.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>
#include <D3DHelperFunctions.hpp>

CPUAccessibleStorage::CPUAccessibleStorage() noexcept
	: m_currentOffset(0u) {}

SharedPair CPUAccessibleStorage::GetSharedAddresses(
	size_t bufferSize
) noexcept {
	constexpr size_t alignment = 256u;

	m_currentOffset = Align(m_currentOffset, alignment);

	m_sharedOffsets.emplace_back(m_currentOffset, m_currentOffset);

	m_currentOffset += bufferSize;

	return m_sharedOffsets.back();
}

void CPUAccessibleStorage::CreateBuffer(ID3D12Device* device) {
	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC rsDesc = CD3DX12_RESOURCE_DESC::Buffer(m_currentOffset);

	HRESULT hr;
	D3D_THROW_FAILED(hr,
		device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&rsDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			__uuidof(ID3D12Resource),
			&m_buffer
		)
	);

	size_t cpuStart = 0u;
	size_t gpuStart = static_cast<size_t>(m_buffer->GetGPUVirtualAddress());

	{
		std::uint8_t* pCPUHandle = nullptr;

		m_buffer->Map(1u, nullptr, reinterpret_cast<void**>(&pCPUHandle));
		cpuStart = static_cast<size_t>(reinterpret_cast<std::uint64_t>(pCPUHandle));
	}

	for (size_t index = 0u; index < m_sharedOffsets.size(); ++index) {
		SharedPair& sharedOffset = m_sharedOffsets[index];

		sharedOffset.first += cpuStart;
		sharedOffset.second += gpuStart;
	}
}
