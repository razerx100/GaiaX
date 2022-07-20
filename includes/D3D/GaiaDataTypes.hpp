#ifndef GAIA_DATA_TYPES_HPP_
#define GAIA_DATA_TYPES_HPP_
#include <memory>
#include <D3DHeaders.hpp>
#include <ShareableAddress.hpp>

using ShareableGPUAddress = _ShareableAddress<D3D12_GPU_VIRTUAL_ADDRESS>;
using ShareableCPUHandle = _ShareableAddress<SIZE_T>;
using ShareableGPUHandle = _ShareableAddress<UINT64>;
using D3DGPUSharedAddress = std::shared_ptr<ShareableGPUAddress>;
using SharedCPUHandle = std::shared_ptr<ShareableCPUHandle>;
using SharedGPUHandle = std::shared_ptr<ShareableGPUHandle>;
using SharedAddress = std::shared_ptr<ShareableAddress>;

struct SharedAddressPair {
	SharedAddress cpuAddress;
	SharedAddress gpuAddress;
};

struct Resolution {
	std::uint64_t width;
	std::uint64_t height;
};
#endif
