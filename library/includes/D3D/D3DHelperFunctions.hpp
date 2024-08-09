#ifndef D3D_HELPER_FUNCTIONS_HPP_
#define D3D_HELPER_FUNCTIONS_HPP_
#include <D3DHeaders.hpp>
#include <utility>
#include <GaiaDataTypes.hpp>

[[nodiscard]]
Resolution GetDisplayResolution(
	ID3D12Device* gpu, IDXGIFactory1* factory, std::uint32_t displayIndex
);

[[nodiscard]]
D3D12_RESOURCE_BARRIER GetTransitionBarrier(
    ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState,
    D3D12_RESOURCE_STATES afterState
) noexcept;

[[nodiscard]]
constexpr size_t Align(size_t address, size_t alignment) noexcept {
	return (address + (alignment - 1u)) & ~(alignment - 1u);
}
#endif
