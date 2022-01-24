#ifndef __I_VIEWPORT_AND_SCISSOR_MANAGER_HPP__
#define __I_VIEWPORT_AND_SCISSOR_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <cstdint>

class IViewportAndScissorManager {
public:
	virtual ~IViewportAndScissorManager() = default;
	virtual const D3D12_VIEWPORT* GetViewportRef() const noexcept = 0;
	virtual const D3D12_RECT* GetScissorRef() const noexcept = 0;

	virtual void Resize(std::uint32_t width, std::uint32_t height) noexcept = 0;
};

IViewportAndScissorManager* CreateViewportAndScissorInstance(
	std::uint32_t width, std::uint32_t height
);

#endif