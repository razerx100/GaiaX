#include <ViewportAndScissorManager.hpp>

ViewportAndScissorManager::ViewportAndScissorManager() noexcept : m_viewport{}, m_scissor{} {
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissor.left = 0l;
	m_scissor.top = 0l;
}

const D3D12_VIEWPORT* ViewportAndScissorManager::GetViewportRef() const noexcept {
	return &m_viewport;
}

const D3D12_RECT* ViewportAndScissorManager::GetScissorRef() const noexcept {
	return &m_scissor;
}

void ViewportAndScissorManager::Resize(std::uint32_t width, std::uint32_t height) noexcept {
	ResizeViewport(width, height);
	ResizeScissor(width, height);
}

void ViewportAndScissorManager::ResizeViewport(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_viewport.Width = static_cast<FLOAT>(width);
	m_viewport.Height = static_cast<FLOAT>(height);
}

void ViewportAndScissorManager::ResizeScissor(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_scissor.right = static_cast<LONG>(width);
	m_scissor.bottom = static_cast<LONG>(height);
}
