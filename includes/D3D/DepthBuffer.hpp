#ifndef DEPTH_BUFFER_HPP_
#define DEPTH_BUFFER_HPP_
#include <D3DHeaders.hpp>
#include <cstdint>

class DepthBuffer {
public:
	DepthBuffer(ID3D12Device* device);

	void CreateDepthBuffer(
		ID3D12Device* device,
		std::uint32_t width, std::uint32_t height
	);

	void ClearDSV(
		ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle
	) noexcept;

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const noexcept;

private:
	ComPtr<ID3D12DescriptorHeap> m_pDSVHeap;
	ComPtr<ID3D12Resource> m_pDepthBuffer;
};
#endif
