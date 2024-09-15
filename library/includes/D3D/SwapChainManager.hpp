#ifndef SWAP_CHAIN_MANAGER_HPP_
#define SWAP_CHAIN_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <array>
#include <D3DResources.hpp>
#include <D3DDescriptorHeapManager.hpp>
#include <D3DCommandQueue.hpp>

class SwapchainManager
{
public:
	SwapchainManager(D3DReusableDescriptorHeap* rtvHeap)
		: m_swapchain{}, m_rtvHeap{ rtvHeap }, m_renderTargets{},
		m_descriptorIndices{}, m_presentFlag{ 0u }
	{}
	SwapchainManager(
		D3DReusableDescriptorHeap* rtvHeap,
		IDXGIFactory5* factory, const D3DCommandQueue& presentQueue, HWND windowHandle, UINT bufferCount
	);
	~SwapchainManager() noexcept;

	void Create(
		IDXGIFactory5* factory, const D3DCommandQueue& presentQueue, HWND windowHandle, UINT bufferCount
	);

	void Resize(UINT width, UINT height);
	void ClearRTV(
		const D3DCommandList& commandList, size_t frameIndex, const std::array<float, 4u>& clearColour
	);

	void Present() { m_swapchain->Present(0u, m_presentFlag); }

	[[nodiscard]]
	UINT GetCurrentBackBufferIndex() const noexcept
	{
		return m_swapchain->GetCurrentBackBufferIndex();
	}
	[[nodiscard]]
	ID3D12Resource* GetRenderTarget(size_t frameIndex) const noexcept
	{
		return m_renderTargets[frameIndex].Get();
	}

private:
	void CreateRTVs();

private:
	ComPtr<IDXGISwapChain4>             m_swapchain;
	D3DReusableDescriptorHeap*          m_rtvHeap;
	std::vector<ComPtr<ID3D12Resource>> m_renderTargets;
	std::vector<UINT>                   m_descriptorIndices;
	UINT                                m_presentFlag;

public:
	SwapchainManager(const SwapchainManager&) = delete;
	SwapchainManager& operator=(const SwapchainManager&) = delete;

	SwapchainManager(SwapchainManager&& other) noexcept
		: m_swapchain{ std::move(other.m_swapchain) },
		m_rtvHeap{ other.m_rtvHeap },
		m_renderTargets{ std::move(other.m_renderTargets) },
		m_descriptorIndices{ std::move(other.m_descriptorIndices) },
		m_presentFlag{ other.m_presentFlag }
	{}
	SwapchainManager& operator=(SwapchainManager&& other) noexcept
	{
		m_swapchain         = std::move(other.m_swapchain);
		m_rtvHeap           = other.m_rtvHeap;
		m_renderTargets     = std::move(other.m_renderTargets);
		m_descriptorIndices = std::move(other.m_descriptorIndices);
		m_presentFlag       = other.m_presentFlag;

		return *this;
	}
};
#endif
