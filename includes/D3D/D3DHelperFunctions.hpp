#ifndef D3D_HELPER_FUNCTIONS_HPP_
#define D3D_HELPER_FUNCTIONS_HPP_
#include <D3DHeaders.hpp>
#include <utility>

using Resolution = std::pair<std::uint64_t, std::uint64_t>;

[[nodiscard]]
Resolution GetDisplayResolution(
	ID3D12Device* gpu, IDXGIFactory1* factory,
	std::uint32_t displayIndex
);

[[nodiscard]]
constexpr size_t Align(size_t address, size_t alignment) noexcept {
	return (address + (alignment - 1u)) & ~(alignment - 1u);
}

constexpr size_t operator"" _B(unsigned long long number) noexcept {
    return static_cast<size_t>(number);
}

constexpr size_t operator"" _KB(unsigned long long number) noexcept {
    return static_cast<size_t>(number * 1024u);
}

constexpr size_t operator"" _MB(unsigned long long number) noexcept {
    return static_cast<size_t>(number * 1024u * 1024u);
}

constexpr size_t operator"" _GB(unsigned long long number) noexcept {
    return static_cast<size_t>(number * 1024u * 1024u * 1024u);
}
#endif
