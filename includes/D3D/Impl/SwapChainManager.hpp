#ifndef __SWAP_CHAIN_MANAGER_HPP__
#define __SWAP_CHAIN_MANAGER_HPP__
#include <ISwapChainManager.hpp>
#include <vector>

class SwapChainManager : public ISwapChainManager {
public:
	SwapChainManager(
		IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
		std::uint8_t bufferCount,
		std::uint32_t width, std::uint32_t height,
		bool variableRefreshRateAvailable
	);

	void ToggleVSync() noexcept override;
	void PresentWithTear() override;
	void PresentWithoutTear() override;
	void Resize(std::uint32_t width, std::uint32_t height) override;

	D3D12_CPU_DESCRIPTOR_HANDLE ClearRTV(
		ID3D12GraphicsCommandList* commandList, float* clearColor
	) noexcept override;
	std::uint32_t GetCurrentBackBufferIndex() const noexcept override;
	D3D12_RESOURCE_BARRIER GetRenderStateBarrier() const noexcept override;
	D3D12_RESOURCE_BARRIER GetPresentStateBarrier() const noexcept override;

	IDXGISwapChain4* GetRef() const noexcept override;

private:
	void CreateRTVHeap(std::uint8_t bufferCount);
	void CreateRTVs();

private:
	std::uint32_t m_width;
	std::uint32_t m_height;
	std::uint32_t m_rtvDescSize;
	bool m_vsyncFlag;
	ComPtr<IDXGISwapChain4> m_pSwapChain;
	std::vector<ComPtr<ID3D12Resource>> m_pRenderTargetViews;
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
};
#endif
