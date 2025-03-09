#ifndef GAIA_HPP_
#define GAIA_HPP_
#include <DeviceManager.hpp>
#include <SwapchainManager.hpp>
#include <RendererTypes.hpp>
#include <RenderEngine.hpp>

class Gaia
{
public:
	Gaia(
		void* windowHandle, UINT width, UINT height, std::uint32_t bufferCount,
		std::shared_ptr<ThreadPool> threadPool, RenderEngineType engineType
	);

	void FinaliseInitialisation();

	void Resize(UINT width, UINT height);
	void Render();

	// Instead of creating useless wrapper functions, let's just return a reference to the RenderEngine.
	[[nodiscard]]
	RenderEngine& GetRenderEngine() noexcept { return *m_renderEngine; }
	[[nodiscard]]
	const RenderEngine& GetRenderEngine() const noexcept { return *m_renderEngine; }

	[[nodiscard]]
	DeviceManager::Resolution GetFirstDisplayCoordinates() const;

	[[nodiscard]]
	SwapchainManager::Extent GetCurrentRenderArea() const noexcept;

private:
	void CreateDevice();
	void CreateRenderEngine(
		RenderEngineType engineType, std::shared_ptr<ThreadPool>&& threadPool, std::uint32_t frameCount
	);
	void CreateSwapchain(std::uint32_t frameCount, void* windowHandle);

private:
	DeviceManager                              m_deviceManager;
	// Putting the RenderEngine before the swapchain, because the swapchain
	// requires a presentation queue. I am using the graphics queue as the
	// presentation engine/queue.
	std::unique_ptr<RenderEngine>              m_renderEngine;
	// Swapchain and RTVHeap are unique_ptrs because the ReusableHeap ctor requires a valid device.
	// Which is created later. And the swapchain requires the RTV heap.
	std::unique_ptr<D3DReusableDescriptorHeap> m_rtvHeap;
	std::unique_ptr<SwapchainManager>          m_swapchain;
	UINT                                       m_windowWidth;
	UINT                                       m_windowHeight;

	static constexpr D3D_FEATURE_LEVEL s_featureLevel = D3D_FEATURE_LEVEL_12_0;

public:
	Gaia(const Gaia&) = delete;
	Gaia& operator=(const Gaia&) = delete;

	Gaia(Gaia&& other) noexcept
		: m_deviceManager{ std::move(other.m_deviceManager) },
		m_renderEngine{ std::move(other.m_renderEngine) },
		m_rtvHeap{ std::move(other.m_rtvHeap) },
		m_swapchain{ std::move(other.m_swapchain) },
		m_windowWidth{ other.m_windowWidth },
		m_windowHeight{ other.m_windowHeight }
	{}
	Gaia& operator=(Gaia&& other) noexcept
	{
		m_deviceManager = std::move(other.m_deviceManager);
		m_renderEngine  = std::move(other.m_renderEngine);
		m_rtvHeap       = std::move(other.m_rtvHeap);
		m_swapchain     = std::move(other.m_swapchain);
		m_windowWidth   = other.m_windowWidth;
		m_windowHeight  = other.m_windowHeight;

		return *this;
	}
};
#endif
