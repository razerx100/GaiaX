#ifndef __DEPTH_BUFFER_HPP__
#define __DEPTH_BUFFER_HPP__
#include <IDepthBuffer.hpp>

class DepthBuffer : public IDepthBuffer {
public:
	DepthBuffer(ID3D12Device* device);

	void CreateDepthBuffer(
		ID3D12Device* device,
		std::uint32_t width, std::uint32_t height
	) override;

	void ClearDSV(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) noexcept override;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const noexcept override;

private:
	ComPtr<ID3D12DescriptorHeap> m_pDSVHeap;
	ComPtr<ID3D12Resource> m_pDepthBuffer;
};
#endif
