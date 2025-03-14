#ifndef D3D_RENDER_PASS_MANAGER_HPP_
#define D3D_RENDER_PASS_MANAGER_HPP_
#include <array>
#include <utility>
#include <D3DResourceBarrier.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DRenderingAttachments.hpp>

class D3DRenderPassManager
{
	struct DepthStencilInfo
	{
		float        depthClearColour;
		std::uint8_t stencilClearColour;
		std::uint8_t dsvFlags;
		std::uint8_t clearFlags;
	};

public:
	using RTVClearColour = std::array<float, 4u>;

public:
	D3DRenderPassManager(D3DReusableDescriptorHeap* rtvHeap, D3DReusableDescriptorHeap* dsvHeap)
		: m_rtvHeap{ rtvHeap }, m_renderTargets{}, m_rtvHandles{}, m_rtvClearColours{},
		m_depthStencilTarget{ dsvHeap }, m_dsvHandle{}, m_startImageBarriers{},
		m_depthStencilInfo{
			.depthClearColour = 1.f, .stencilClearColour = 0u, .dsvFlags = 0u, .clearFlags = 0u
		}
	{}

	void AddRenderTarget(
		ID3D12Resource* renderTargetResource, DXGI_FORMAT rtvFormat, bool clearAtStart
	);
	[[nodiscard]]
	std::uint32_t AddStartBarrier(const ResourceBarrierBuilder& barrierBuilder) noexcept;

	void RecreateRenderTarget(
		size_t renderTargetIndex, size_t barrierIndex, ID3D12Resource* renderTargetResource,
		DXGI_FORMAT rtvFormat
	);

	void SetDepthStencilTarget(
		ID3D12Resource* depthStencilTargetResource, DXGI_FORMAT dsvFormat,
		bool depthClearAtStart, bool stencilClearAtStart, D3D12_DSV_FLAGS dsvFlags
	);

	void RecreateDepthStencilTarget(
		size_t barrierIndex, ID3D12Resource* depthStencilTargetResource, DXGI_FORMAT dsvFormat
	);

	// These functions can be used every frame.
	void SetTransitionAfterState(size_t barrierIndex, D3D12_RESOURCE_STATES afterState) noexcept;

	void SetDepthClearValue(float depthClearValue) noexcept;
	void SetStencilClearValue(std::uint8_t stencilClearValue) noexcept;

	void SetRenderTargetClearValue(
		size_t renderTargetIndex, const RTVClearColour& clearValue
	) noexcept;

	void StartPass(const D3DCommandList& graphicsCmdList) const noexcept;

	// Only need to end pass for the swapchain, as in Dx12 we don't need any specific end rendering
	// API call.
	void EndPassForSwapchain(
		const D3DCommandList& graphicsCmdList, ID3D12Resource* srcRenderTarget,
		ID3D12Resource* swapchainBackBuffer
	) const noexcept;

private:
	D3DReusableDescriptorHeap*               m_rtvHeap;
	std::vector<RenderTarget>                m_renderTargets;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvHandles;
	std::vector<RTVClearColour>              m_rtvClearColours;
	DepthStencilTarget                       m_depthStencilTarget;
	D3D12_CPU_DESCRIPTOR_HANDLE              m_dsvHandle;
	D3DResourceBarrier1_1                    m_startImageBarriers;
	DepthStencilInfo                         m_depthStencilInfo;

public:
	D3DRenderPassManager(const D3DRenderPassManager&) = delete;
	D3DRenderPassManager& operator=(const D3DRenderPassManager&) = delete;

	D3DRenderPassManager(D3DRenderPassManager&& other) noexcept
		: m_rtvHeap{ std::exchange(other.m_rtvHeap, nullptr) },
		m_renderTargets{ std::move(other.m_renderTargets) },
		m_rtvHandles{ std::move(other.m_rtvHandles) },
		m_rtvClearColours{ std::move(other.m_rtvClearColours) },
		m_depthStencilTarget{ std::move(other.m_depthStencilTarget) },
		m_dsvHandle{ other.m_dsvHandle },
		m_startImageBarriers{ std::move(other.m_startImageBarriers) },
		m_depthStencilInfo{ other.m_depthStencilInfo }
	{}
	D3DRenderPassManager& operator=(D3DRenderPassManager&& other) noexcept
	{
		m_rtvHeap            = std::exchange(other.m_rtvHeap, nullptr);
		m_renderTargets      = std::move(other.m_renderTargets);
		m_rtvHandles         = std::move(other.m_rtvHandles);
		m_rtvClearColours    = std::move(other.m_rtvClearColours);
		m_depthStencilTarget = std::move(other.m_depthStencilTarget);
		m_dsvHandle          = other.m_dsvHandle;
		m_startImageBarriers = std::move(other.m_startImageBarriers);
		m_depthStencilInfo   = other.m_depthStencilInfo;

		return *this;
	}
};
#endif
