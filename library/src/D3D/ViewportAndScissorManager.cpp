#include <ViewportAndScissorManager.hpp>

ViewportAndScissorManager::ViewportAndScissorManager()
	: m_viewport
	{
		.TopLeftX = 0.f,
		.TopLeftY = 0.f,
		.Width    = 0.f,
		.Height   = 0.f,
		.MinDepth = 0.f,
		.MaxDepth = 1.f
	},
	m_scissor
	{
		.left   = 0l,
		.top    = 0l,
		.right  = 0l,
		.bottom = 0l
	}
{}

void ViewportAndScissorManager::Resize(std::uint32_t width, std::uint32_t height) noexcept
{
	ResizeViewport(width, height);
	ResizeScissor(width, height);
}

void ViewportAndScissorManager::ResizeViewport(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_viewport.Width  = static_cast<FLOAT>(width);
	m_viewport.Height = static_cast<FLOAT>(height);
}

void ViewportAndScissorManager::ResizeScissor(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_scissor.right  = static_cast<LONG>(width);
	m_scissor.bottom = static_cast<LONG>(height);
}

void ViewportAndScissorManager::BindViewportAndScissor(const D3DCommandList& d3dCommandList) const noexcept
{
	ID3D12GraphicsCommandList* commandList = d3dCommandList.Get();

	commandList->RSSetViewports(1u, &m_viewport);
	commandList->RSSetScissorRects(1u, &m_scissor);
}
