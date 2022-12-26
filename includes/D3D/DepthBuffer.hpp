#ifndef DEPTH_BUFFER_HPP_
#define DEPTH_BUFFER_HPP_
#include <D3DHeaders.hpp>
#include <cstdint>
#include <D3DResource.hpp>
#include <optional>

class DepthBuffer {
public:
	struct Args {
		std::optional<ID3D12Device*> device;
	};

public:
	DepthBuffer(const Args& arguments);

	void CreateDepthBuffer(ID3D12Device* device, std::uint32_t width, std::uint32_t height);
	void ReserveHeapSpace(ID3D12Device* device) noexcept;

	void SetMaxResolution(std::uint32_t width, std::uint32_t height) noexcept;

	void ClearDSV(
		ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle
	) noexcept;

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const noexcept;

private:
	ComPtr<ID3D12DescriptorHeap> m_pDSVHeap;
	D3DResourceView m_depthBuffer;
	std::uint32_t m_maxWidth;
	std::uint32_t m_maxHeight;
};
#endif
