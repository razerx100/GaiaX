#ifndef D3D_VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#define D3D_VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <cstdint>
#include <D3DCommandQueue.hpp>

namespace Gaia
{
class ViewportAndScissorManager
{
public:
	ViewportAndScissorManager();

	void Resize(std::uint32_t width, std::uint32_t height) noexcept;
	void Bind(const D3DCommandList& d3dCommandList) const noexcept;

private:
	void ResizeViewport(std::uint32_t width, std::uint32_t height) noexcept;
	void ResizeScissor(std::uint32_t width, std::uint32_t height) noexcept;

private:
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT     m_scissor;
};
}
#endif