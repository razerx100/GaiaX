#ifndef __CPU_ACCESSIBLE_STORAGE_HPP__
#define __CPU_ACCESSIBLE_STORAGE_HPP__
#include <ICPUAccessibleStorage.hpp>
#include <vector>

class CPUAccessibleStorage : public ICPUAccessibleStorage {
public:
	CPUAccessibleStorage() noexcept;

	[[nodiscard]]
	SharedPair GetSharedAddresses(
		size_t bufferSize
	) noexcept override;

	void CreateBuffer(ID3D12Device* device) override;

private:
	ComPtr<ID3D12Resource> m_buffer;
	std::vector<SharedPair> m_sharedOffsets;
	size_t m_currentOffset;
};
#endif
