#ifndef GAIA_DATA_TYPES_HPP_
#define GAIA_DATA_TYPES_HPP_
#include <cstdint>
#include <D3DHeaders.hpp>

struct Resolution {
	std::uint64_t width;
	std::uint64_t height;
};

struct ModelDrawArguments {
	std::uint32_t modelIndex;
	D3D12_DRAW_INDEXED_ARGUMENTS drawIndexed;
};
#endif
