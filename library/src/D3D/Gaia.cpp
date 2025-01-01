#include <Gaia.hpp>
#include <RenderEngineVS.hpp>
#include <RenderEngineMS.hpp>

Gaia::Gaia(
	void* windowHandle, UINT width, UINT height, std::uint32_t bufferCount,
	std::shared_ptr<ThreadPool> threadPool, RenderEngineType engineType
) : m_deviceManager{}, m_renderEngine{}, m_rtvHeap{}, m_swapchain{},
	m_windowWidth{ 0u }, m_windowHeight{ 0u }
{
	CreateDevice();

	CreateRenderEngine(engineType, std::move(threadPool), bufferCount);

	CreateSwapchain(bufferCount, windowHandle);

	// Need to create the render targets and stuffs.
	Resize(width, height);
}

void Gaia::CreateDevice()
{
#if _DEBUG
	m_deviceManager.GetDebugLogger().AddCallbackType(DebugCallbackType::FileOut);
#endif
	m_deviceManager.Create(s_featureLevel);
}

void Gaia::CreateRenderEngine(
	RenderEngineType engineType, std::shared_ptr<ThreadPool>&& threadPool, std::uint32_t frameCount
) {
	if (engineType == RenderEngineType::IndividualDraw)
		m_renderEngine = std::make_unique<RenderEngineVSIndividual>(
			m_deviceManager, std::move(threadPool), frameCount
		);
	else if (engineType == RenderEngineType::IndirectDraw)
		m_renderEngine = std::make_unique<RenderEngineVSIndirect>(
			m_deviceManager, std::move(threadPool), frameCount
		);
	else if (engineType == RenderEngineType::MeshDraw)
		m_renderEngine = std::make_unique<RenderEngineMS>(
			m_deviceManager, std::move(threadPool), frameCount
		);
}

void Gaia::CreateSwapchain(std::uint32_t frameCount, void* windowHandle)
{
	ID3D12Device5* device = m_deviceManager.GetDevice();

	m_rtvHeap = std::make_unique<D3DReusableDescriptorHeap>(
		device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	);

	m_swapchain = std::make_unique<SwapchainManager>(
		m_rtvHeap.get(), frameCount, m_deviceManager.GetFactory(),
		m_renderEngine->GetPresentQueue(), static_cast<HWND>(windowHandle)
	);
}

void Gaia::Resize(UINT width, UINT height)
{
	// Only recreate these if the new resolution is different.
	if (m_windowWidth != width || m_windowHeight != height)
	{
		m_renderEngine->WaitForGPUToFinish();

		// Must resize the swapchain first.
		m_swapchain->Resize(width, height);

		m_renderEngine->Resize(width, height);

		m_windowWidth  = width;
		m_windowHeight = height;
	}
}

void Gaia::Render()
{
	const size_t nextImageIndex          = m_swapchain->GetCurrentBackBufferIndex();
	const RenderTarget& nextRenderTarget = m_swapchain->GetRenderTarget(nextImageIndex);

	m_renderEngine->Render(nextImageIndex, nextRenderTarget);

	m_swapchain->Present();
}

DeviceManager::Resolution Gaia::GetFirstDisplayCoordinates() const
{
	return m_deviceManager.GetDisplayResolution(0u);
}
