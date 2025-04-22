#ifndef GAIA_HPP_
#define GAIA_HPP_
#include <D3DDeviceManager.hpp>
#include <D3DSwapchainManager.hpp>
#include <RendererTypes.hpp>
#include <D3DRenderEngineVS.hpp>
#include <D3DRenderEngineMS.hpp>

namespace Gaia
{
template<class RenderEngine_t>
class Gaia
{
public:
	Gaia(
		void* windowHandle, UINT width, UINT height, std::uint32_t bufferCount,
		std::shared_ptr<ThreadPool> threadPool
	) : m_deviceManager{ CreateDevice() },
		m_rtvHeap{
			std::make_unique<D3DReusableDescriptorHeap>(
				m_deviceManager.GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE
			)
		},
		m_renderEngine{ m_deviceManager, std::move(threadPool), bufferCount },
		m_swapchain{
			m_rtvHeap.get(), bufferCount, m_deviceManager.GetFactory(),
			m_renderEngine.GetPresentQueue(), static_cast<HWND>(windowHandle)
		},
		m_windowWidth{ 0u }, m_windowHeight{ 0u }
	{
		Resize(width, height);
	}

	void FinaliseInitialisation()
	{
		m_renderEngine.FinaliseInitialisation(m_deviceManager);
	}

	void Resize(UINT width, UINT height)
	{
		// Only recreate these if the new resolution is different.
		if (m_windowWidth != width || m_windowHeight != height)
		{
			m_renderEngine.WaitForGPUToFinish();

			// Must resize the swapchain first.
			m_swapchain.Resize(width, height);

			m_renderEngine.Resize(width, height);

			m_windowWidth  = width;
			m_windowHeight = height;
		}
	}

	void Render()
	{
		const size_t nextImageIndex      = m_swapchain.GetCurrentBackBufferIndex();
		ID3D12Resource* nextRenderTarget = m_swapchain.GetRenderTarget(nextImageIndex);

		m_renderEngine.Render(nextImageIndex, nextRenderTarget);

		m_swapchain.Present();
	}

	// Instead of creating useless wrapper functions, let's just return a reference to the
	// RenderEngine.
	[[nodiscard]]
	auto&& GetRenderEngine(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(self.m_renderEngine);
	}

	[[nodiscard]]
	ExternalFormat GetSwapchainFormat() const noexcept
	{
		return GetExternalFormat(m_swapchain.GetFormat());
	}

	[[nodiscard]]
	std::uint32_t AddExternalRenderPass()
	{
		return m_renderEngine.AddExternalRenderPass(m_rtvHeap.get());
	}

	void SetSwapchainExternalRenderPass()
	{
		m_renderEngine.SetSwapchainExternalRenderPass(m_rtvHeap.get());
	}

	[[nodiscard]]
	DeviceManager::Resolution GetFirstDisplayCoordinates() const
	{
		return m_deviceManager.GetDisplayResolution(0u);
	}

	[[nodiscard]]
	SwapchainManager::Extent GetCurrentRenderArea() const noexcept
	{
		return m_swapchain.GetCurrentRenderArea();
	}

private:
	[[nodiscard]]
	static DeviceManager CreateDevice()
	{
		DeviceManager deviceManager{};

#ifndef NDEBUG
		deviceManager.GetDebugLogger().AddCallbackType(DebugCallbackType::FileOut);
#endif
		deviceManager.Create(s_featureLevel);

		return deviceManager;
	}

private:
	DeviceManager                              m_deviceManager;
	// The rtv heap must be before the render engine, so the render engine is destroyed first and
	// no external render targets have a dangling pointer.
	// This needs to be a unique/shared_ptr as it is shared.
	std::unique_ptr<D3DReusableDescriptorHeap> m_rtvHeap;
	// Putting the RenderEngine before the swapchain, because the swapchain
	// requires a presentation queue. I am using the graphics queue as the
	// presentation engine/queue.
	RenderEngine_t                             m_renderEngine;
	SwapchainManager                           m_swapchain;
	UINT                                       m_windowWidth;
	UINT                                       m_windowHeight;

	static constexpr D3D_FEATURE_LEVEL s_featureLevel = D3D_FEATURE_LEVEL_12_0;

public:
	Gaia(const Gaia&) = delete;
	Gaia& operator=(const Gaia&) = delete;

	Gaia(Gaia&& other) noexcept
		: m_deviceManager{ std::move(other.m_deviceManager) },
		m_rtvHeap{ std::move(other.m_rtvHeap) },
		m_renderEngine{ std::move(other.m_renderEngine) },
		m_swapchain{ std::move(other.m_swapchain) },
		m_windowWidth{ other.m_windowWidth },
		m_windowHeight{ other.m_windowHeight }
	{}
	Gaia& operator=(Gaia&& other) noexcept
	{
		m_deviceManager = std::move(other.m_deviceManager);
		m_rtvHeap       = std::move(other.m_rtvHeap);
		m_renderEngine  = std::move(other.m_renderEngine);
		m_swapchain     = std::move(other.m_swapchain);
		m_windowWidth   = other.m_windowWidth;
		m_windowHeight  = other.m_windowHeight;

		return *this;
	}
};
}
#endif
