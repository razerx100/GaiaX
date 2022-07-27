#ifndef CPU_ACCESSIBLE_STORAGE_HPP_
#define CPU_ACCESSIBLE_STORAGE_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <GaiaDataTypes.hpp>
#include <D3DResource.hpp>

class CPUAccessibleStorage {
public:
	CPUAccessibleStorage() noexcept;

	[[nodiscard]]
	SharedAddressPair GetSharedAddresses(size_t bufferSize) noexcept;

	void CreateBuffer(ID3D12Device* device);
	void ReserveHeapSpace() noexcept;

private:
	D3DCPUWResource m_buffer;
	std::vector<SharedAddressPair> m_sharedOffsets;
	size_t m_currentOffset;
	size_t m_heapOffset;
};
#endif
