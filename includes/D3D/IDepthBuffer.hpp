#ifndef __I_DEPTH_BUFFER_HPP__
#define __I_DEPTH_BUFFER_HPP__
#include <D3DHeaders.hpp>
#include <cstdint>

class IDepthBuffer {
public:
	virtual ~IDepthBuffer() = default;

	virtual void CreateDepthBuffer(
		ID3D12Device* device,
		std::uint32_t width, std::uint32_t height
	) = 0;

	virtual void ClearDSV(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const noexcept = 0;
};

IDepthBuffer* CreateDepthBufferInstance(ID3D12Device* device);

#endif
