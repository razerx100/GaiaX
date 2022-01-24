#ifndef __VIEWPORT_AND_SCISSOR_MANAGER_HPP__
#define __VIEWPORT_AND_SCISSOR_MANAGER_HPP__
#include <IViewportAndScissorManager.hpp>

class ViewportAndScissorManager : public IViewportAndScissorManager {
public:
	ViewportAndScissorManager(std::uint32_t width, std::uint32_t height);

	const D3D12_VIEWPORT* GetViewportRef() const noexcept override;
	const D3D12_RECT* GetScissorRef() const noexcept override;

	void Resize(std::uint32_t width, std::uint32_t height) noexcept override;

private:
	void ResizeViewport(std::uint32_t width, std::uint32_t height) noexcept;
	void ResizeScissor(std::uint32_t width, std::uint32_t height) noexcept;

private:
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissor;
};
#endif