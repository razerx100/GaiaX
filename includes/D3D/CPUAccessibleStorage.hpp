#ifndef CPU_ACCESSIBLE_STORAGE_HPP_
#define CPU_ACCESSIBLE_STORAGE_HPP_
#include <D3DHeaders.hpp>
#include <SharedAddress.hpp>
#include <vector>

using SharedPair = std::pair<SharedAddress, SharedAddress>;

class CPUAccessibleStorage {
public:
	CPUAccessibleStorage() noexcept;

	[[nodiscard]]
	SharedPair GetSharedAddresses(
		size_t bufferSize
	) noexcept;

	void CreateBuffer(ID3D12Device* device);

private:
	ComPtr<ID3D12Resource> m_buffer;
	std::vector<SharedPair> m_sharedOffsets;
	size_t m_currentOffset;
};
#endif
