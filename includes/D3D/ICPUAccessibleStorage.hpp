#ifndef __I_CPU_ACCESSIBLE_STORAGE_HPP__
#define __I_CPU_ACCESSIBLE_STORAGE_HPP__
#include <D3DHeaders.hpp>
#include <SharedAddress.hpp>
#include <cstdint>
#include <memory>

using SharedPair = std::pair<SharedAddress, SharedAddress>;

class ICPUAccessibleStorage {
public:
	virtual ~ICPUAccessibleStorage() = default;

	[[nodiscard]]
	virtual SharedPair GetSharedAddresses(
		size_t bufferSize
	) noexcept = 0;

	virtual void CreateBuffer(ID3D12Device* device) = 0;
};
#endif
