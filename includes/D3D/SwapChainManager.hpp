#ifndef __SWAP_CHAIN_MANAGER_HPP__
#define __SWAP_CHAIN_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <vector>
#include <cstdint>

class SwapChainManager {
public:
	SwapChainManager(
		IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
		std::uint8_t bufferCount,
		std::uint32_t width, std::uint32_t height,
		bool variableRefreshRateAvailable
	);

	void ToggleVSync() noexcept;
	void PresentWithTear();
	void PresentWithoutTear();
	// Should be only called when all of the back buffers have finished executing
	void Resize(std::uint32_t width, std::uint32_t height);

	std::uint32_t GetCurrentBackBufferIndex() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(std::uint8_t frameIndex) noexcept;
	D3D12_RESOURCE_BARRIER GetRenderStateBarrier(std::uint8_t frameIndex) noexcept;
	D3D12_RESOURCE_BARRIER GetPresentStateBarrier(std::uint8_t frameIndex) noexcept;

private:
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

SwapChainManager* GetSwapChainInstance() noexcept;
void InitSwapChianInstance(
	IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
	std::uint8_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	bool variableRefreshRateAvailable = true
);
void CleanUpSwapChainInstance() noexcept;
#endif
