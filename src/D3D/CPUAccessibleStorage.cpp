#include <CPUAccessibleStorage.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>
#include <D3DHelperFunctions.hpp>

CPUAccessibleStorage::CPUAccessibleStorage() noexcept : m_currentOffset(0u) {}

SharedAddressPair CPUAccessibleStorage::GetSharedAddresses(size_t bufferSize) noexcept {
	constexpr size_t alignment = 256u;

	m_currentOffset = Align(m_currentOffset, alignment);

	auto cpuAddress = std::make_shared<ShareableAddress>(m_currentOffset);
	auto gpuAddress = std::make_shared<ShareableAddress>(m_currentOffset);

	m_sharedOffsets.emplace_back(cpuAddress, gpuAddress);

	m_currentOffset += bufferSize;

	return { std::move(cpuAddress), std::move(gpuAddress) };
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

		m_buffer->Map(0u, nullptr, reinterpret_cast<void**>(&pCPUHandle));
		cpuStart = reinterpret_cast<size_t>(pCPUHandle);
	}

	for (size_t index = 0u; index < std::size(m_sharedOffsets); ++index) {
		auto& [cpuSharedOffset, gpuSharedOffset] = m_sharedOffsets[index];

		*cpuSharedOffset += cpuStart;
		*gpuSharedOffset += gpuStart;
	}

	m_sharedOffsets = std::vector<SharedAddressPair>();
}
