#ifndef __I_SWAP_CHAIN_MANAGER_HPP__
#define __I_SWAP_CHAIN_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <cstdint>

class ISwapChainManager {
public:
	virtual ~ISwapChainManager() = default;

	virtual void ToggleVSync() noexcept = 0;
	virtual void PresentWithTear() = 0;
	virtual void PresentWithoutTear() = 0;
	// Should be only called when all of the back buffers have finished executing
	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;

	virtual D3D12_CPU_DESCRIPTOR_HANDLE ClearRTV(
		ID3D12GraphicsCommandList* commandList, float* clearColor
	) noexcept = 0;
	virtual std::uint32_t GetCurrentBackBufferIndex() const noexcept = 0;
	virtual D3D12_RESOURCE_BARRIER GetRenderStateBarrier() const noexcept = 0;
	virtual D3D12_RESOURCE_BARRIER GetPresentStateBarrier() const noexcept = 0;

	virtual IDXGISwapChain4* GetRef() const noexcept = 0;
};

ISwapChainManager* GetSwapChainInstance() noexcept;
void InitSwapChianInstance(
	IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue, void* windowHandle,
	std::uint8_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	bool variableRefreshRateAvailable = true
);
void CleanUpSwapChainInstance() noexcept;

#endif
