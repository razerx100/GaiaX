#ifndef CPU_ACCESSIBLE_STORAGE_HPP_
#define CPU_ACCESSIBLE_STORAGE_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <GaiaDataTypes.hpp>

class CPUAccessibleStorage {
public:
	CPUAccessibleStorage() noexcept;

	[[nodiscard]]
	SharedAddressPair GetSharedAddresses(size_t bufferSize) noexcept;

	void CreateBuffer(ID3D12Device* device);

private:
	ComPtr<ID3D12Resource> m_buffer;
	std::vector<SharedAddressPair> m_sharedOffsets;
	size_t m_currentOffset;
};
#endif
