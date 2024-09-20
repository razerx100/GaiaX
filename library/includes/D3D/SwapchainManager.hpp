#ifndef SWAP_CHAIN_MANAGER_HPP_
#define SWAP_CHAIN_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <D3DRenderTarget.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <D3DCommandQueue.hpp>

class SwapchainManager
{
public:
	SwapchainManager(D3DReusableDescriptorHeap* rtvHeap, UINT bufferCount);
	SwapchainManager(
		D3DReusableDescriptorHeap* rtvHeap, UINT bufferCount,
		IDXGIFactory5* factory, const D3DCommandQueue& presentQueue, HWND windowHandle
	);

	void Create(IDXGIFactory5* factory, const D3DCommandQueue& presentQueue, HWND windowHandle);

	void Resize(UINT width, UINT height);

	void Present() { m_swapchain->Present(0u, m_presentFlag); }

	[[nodiscard]]
	UINT GetCurrentBackBufferIndex() const noexcept
	{
		return m_swapchain->GetCurrentBackBufferIndex();
	}
	[[nodiscard]]
	const RenderTarget& GetRenderTarget(size_t frameIndex) const noexcept
	{
		return m_renderTargets[frameIndex];
	}

private:
	void CreateRTVs();

private:
	ComPtr<IDXGISwapChain4>   m_swapchain;
	std::vector<RenderTarget> m_renderTargets;
	UINT                      m_presentFlag;

public:
	SwapchainManager(const SwapchainManager&) = delete;
	SwapchainManager& operator=(const SwapchainManager&) = delete;

	SwapchainManager(SwapchainManager&& other) noexcept
		: m_swapchain{ std::move(other.m_swapchain) },
		m_renderTargets{ std::move(other.m_renderTargets) },
		m_presentFlag{ other.m_presentFlag }
	{}
	SwapchainManager& operator=(SwapchainManager&& other) noexcept
	{
		m_swapchain     = std::move(other.m_swapchain);
		m_renderTargets = std::move(other.m_renderTargets);
		m_presentFlag   = other.m_presentFlag;

		return *this;
	}
};
#endif
