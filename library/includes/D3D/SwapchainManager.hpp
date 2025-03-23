#ifndef SWAP_CHAIN_MANAGER_HPP_
#define SWAP_CHAIN_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <D3DRenderingAttachments.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <D3DCommandQueue.hpp>

class SwapchainManager
{
public:
	struct Extent
	{
		UINT width;
		UINT height;
	};

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
	ID3D12Resource* GetRenderTarget(size_t frameIndex) const noexcept
	{
		return m_renderTargetResources[frameIndex].Get();
	}

	[[nodiscard]]
	Extent GetCurrentRenderArea() const noexcept;

	[[nodiscard]]
	static constexpr DXGI_FORMAT GetFormat() noexcept { return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; }

private:
	void CreateRTVs();

private:
	using ComResource = ComPtr<ID3D12Resource>;

private:
	ComPtr<IDXGISwapChain4>          m_swapchain;
	std::vector<ComResource>         m_renderTargetResources;
	std::vector<RenderingAttachment> m_renderTargets;
	UINT                             m_presentFlag;

public:
	SwapchainManager(const SwapchainManager&) = delete;
	SwapchainManager& operator=(const SwapchainManager&) = delete;

	SwapchainManager(SwapchainManager&& other) noexcept
		: m_swapchain{ std::move(other.m_swapchain) },
		m_renderTargetResources{ std::move(other.m_renderTargetResources) },
		m_renderTargets{ std::move(other.m_renderTargets) },
		m_presentFlag{ other.m_presentFlag }
	{}
	SwapchainManager& operator=(SwapchainManager&& other) noexcept
	{
		m_swapchain             = std::move(other.m_swapchain);
		m_renderTargetResources = std::move(other.m_renderTargetResources);
		m_renderTargets         = std::move(other.m_renderTargets);
		m_presentFlag           = other.m_presentFlag;

		return *this;
	}
};
#endif
