#ifndef VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#define VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <cstdint>

class ViewportAndScissorManager {
public:
	ViewportAndScissorManager() noexcept;

	[[nodiscard]]
	const D3D12_VIEWPORT* GetViewportRef() const noexcept;
	[[nodiscard]]
	const D3D12_RECT* GetScissorRef() const noexcept;

	void Resize(std::uint32_t width, std::uint32_t height) noexcept;

private:
	void ResizeViewport(std::uint32_t width, std::uint32_t height) noexcept;
	void ResizeScissor(std::uint32_t width, std::uint32_t height) noexcept;

private:
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissor;
};
#endif