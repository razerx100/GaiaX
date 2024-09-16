#ifndef D3D_HELPER_FUNCTIONS_HPP_
#define D3D_HELPER_FUNCTIONS_HPP_
#include <D3DHeaders.hpp>
#include <utility>

struct Resolution
{
	std::uint64_t width;
	std::uint64_t height;
};

[[nodiscard]]
Resolution GetDisplayResolution(
	ID3D12Device* gpu, IDXGIFactory1* factory, std::uint32_t displayIndex
);
#endif
