#ifndef SWAP_CHAIN_MANAGER_HPP_
#define SWAP_CHAIN_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <optional>

class SwapChainManager {
public:
	struct Args
	{
		ID3D12Device* device;
		IDXGIFactory2* factory;
		ID3D12CommandQueue* graphicsQueue;
		HWND windowHandle;
		std::uint32_t width;
		std::uint32_t height;
		std::uint32_t bufferCount;
		bool variableRefreshRate;
	};

public:
	SwapChainManager(const Args& arguments);

	void ToggleVSync() noexcept;
	void PresentWithTear();
	void PresentWithoutTear();
	void Resize(
		ID3D12Device* device,
		std::uint32_t width, std::uint32_t height
	);

	void ClearRTV(
		ID3D12GraphicsCommandList* commandList, float* clearColor,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle
	) noexcept;

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(size_t index) const noexcept;
	[[nodiscard]]
	ID3D12Resource* GetRTV(size_t index) const noexcept;
	[[nodiscard]]
	size_t GetCurrentBackBufferIndex() const noexcept;
	[[nodiscard]]
	IDXGISwapChain4* GetRef() const noexcept;

private:
	void CreateRTVHeap(ID3D12Device* device, UINT bufferCount);
	void CreateRTVs(ID3D12Device* device);

private:
	size_t m_rtvDescSize;
	bool m_vsyncFlag;
	ComPtr<IDXGISwapChain4> m_pSwapChain;
	std::vector<ComPtr<ID3D12Resource>> m_pRenderTargetViews;
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
};
#endif
