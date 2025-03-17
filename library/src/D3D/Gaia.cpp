#include <Gaia.hpp>
#include <RenderEngineVS.hpp>
#include <RenderEngineMS.hpp>

Gaia::Gaia(
	void* windowHandle, UINT width, UINT height, std::uint32_t bufferCount,
	std::shared_ptr<ThreadPool> threadPool, RenderEngineType engineType
) : m_deviceManager{}, m_rtvHeap{}, m_renderEngine{}, m_swapchain{}, m_windowWidth{ 0u },
	m_windowHeight{ 0u }
{
	CreateDevice();

	CreateRenderEngine(engineType, std::move(threadPool), bufferCount);

	CreateSwapchain(bufferCount, windowHandle);

	Resize(width, height);
}

void Gaia::FinaliseInitialisation()
{
	m_renderEngine->FinaliseInitialisation(m_deviceManager);
}

void Gaia::CreateDevice()
{
#if _DEBUG
	m_deviceManager.GetDebugLogger().AddCallbackType(DebugCallbackType::FileOut);
#endif
	m_deviceManager.Create(s_featureLevel);

	m_rtvHeap = std::make_unique<D3DReusableDescriptorHeap>(
		m_deviceManager.GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	);
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
	const size_t nextImageIndex      = m_swapchain->GetCurrentBackBufferIndex();
	ID3D12Resource* nextRenderTarget = m_swapchain->GetRenderTarget(nextImageIndex);

	m_renderEngine->Render(nextImageIndex, nextRenderTarget);

	m_swapchain->Present();
}

DeviceManager::Resolution Gaia::GetFirstDisplayCoordinates() const
{
	return m_deviceManager.GetDisplayResolution(0u);
}

SwapchainManager::Extent Gaia::GetCurrentRenderArea() const noexcept
{
	return m_swapchain->GetCurrentRenderArea();
}

std::uint32_t Gaia::AddExternalRenderPass()
{
	return m_renderEngine->AddExternalRenderPass(m_rtvHeap.get());
}

void Gaia::SetSwapchainExternalRenderPass()
{
	m_renderEngine->SetSwapchainExternalRenderPass(m_rtvHeap.get());
}
