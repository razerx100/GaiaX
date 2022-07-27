#include <CPUAccessibleStorage.hpp>
#include <D3DThrowMacros.hpp>
#include <d3dx12.h>
#include <D3DHelperFunctions.hpp>
#include <Gaia.hpp>

CPUAccessibleStorage::CPUAccessibleStorage() noexcept
	: m_currentOffset{ 0u }, m_heapOffset{ 0u } {}

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
	D3D12_RESOURCE_DESC rsDesc = CD3DX12_RESOURCE_DESC::Buffer(m_currentOffset);

	m_buffer.CreateResource(
		device, Gaia::Resources::cpuWriteHeap->GetHeap(), m_heapOffset, rsDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);

	size_t cpuStart = 0u;
	size_t gpuStart = static_cast<size_t>(m_buffer.Get()->GetGPUVirtualAddress());

	{
		m_buffer.MapBuffer();

		std::uint8_t* pCPUHandle = m_buffer.GetCPUWPointer();
		cpuStart = reinterpret_cast<size_t>(pCPUHandle);
	}

	for (size_t index = 0u; index < std::size(m_sharedOffsets); ++index) {
		auto& [cpuSharedOffset, gpuSharedOffset] = m_sharedOffsets[index];

		*cpuSharedOffset += cpuStart;
		*gpuSharedOffset += gpuStart;
	}

	m_sharedOffsets = std::vector<SharedAddressPair>();
}

void CPUAccessibleStorage::ReserveHeapSpace() noexcept {
	m_heapOffset = Gaia::Resources::cpuWriteHeap->ReserveSizeAndGetOffset(
		m_currentOffset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT
	);
}
