#include <RenderEngine.hpp>

RenderEngine::RenderEngine() noexcept :
	m_backgroundColour{ 0.0001f, 0.0001f, 0.0001f, 0.0001f } {}

void RenderEngine::SetBackgroundColour(const std::array<float, 4>& colour) noexcept {
	m_backgroundColour = colour;
}

void RenderEngine::SetShaderPath(const wchar_t* path) noexcept {
	m_shaderPath = path;
}
