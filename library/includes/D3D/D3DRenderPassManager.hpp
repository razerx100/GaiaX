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
		std::uint8_t clearFlags;
	};

public:
	using RTVClearColour = std::array<float, 4u>;

public:
	D3DRenderPassManager()
		: m_rtvHandles{}, m_rtvClearFlags{}, m_rtvClearColours {}, m_dsvHandle{},
		m_depthStencilInfo{ .depthClearColour = 1.f, .stencilClearColour = 0u, .clearFlags = 0u }
	{}

	void AddRenderTarget(
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, bool clearAtStart
	);

	void SetRTVHandle(size_t renderTargetIndex, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);

	void SetDepthStencilTarget(
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, bool depthClearAtStart, bool stencilClearAtStart
	);

	void SetDSVHandle(D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);

	// These functions can be used every frame.
	[[nodiscard]]
	bool IsDepthClearColourSame(float depthClearColour) const noexcept;
	[[nodiscard]]
	bool IsStencilClearColourSame(std::uint8_t stencilClearColour) const noexcept;
	[[nodiscard]]
	bool IsRenderTargetClearColourSame(
		size_t renderTargetIndex, const RTVClearColour& clearValue
	) const noexcept;

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
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvHandles;
	std::vector<bool>                        m_rtvClearFlags;
	std::vector<RTVClearColour>              m_rtvClearColours;
	D3D12_CPU_DESCRIPTOR_HANDLE              m_dsvHandle;
	DepthStencilInfo                         m_depthStencilInfo;

public:
	D3DRenderPassManager(const D3DRenderPassManager&) = delete;
	D3DRenderPassManager& operator=(const D3DRenderPassManager&) = delete;

	D3DRenderPassManager(D3DRenderPassManager&& other) noexcept
		: m_rtvHandles{ std::move(other.m_rtvHandles) },
		m_rtvClearFlags{ std::move(other.m_rtvClearFlags) },
		m_rtvClearColours{ std::move(other.m_rtvClearColours) },
		m_dsvHandle{ other.m_dsvHandle },
		m_depthStencilInfo{ other.m_depthStencilInfo }
	{}
	D3DRenderPassManager& operator=(D3DRenderPassManager&& other) noexcept
	{
		m_rtvHandles       = std::move(other.m_rtvHandles);
		m_rtvClearFlags    = std::move(other.m_rtvClearFlags);
		m_rtvClearColours  = std::move(other.m_rtvClearColours);
		m_dsvHandle        = other.m_dsvHandle;
		m_depthStencilInfo = other.m_depthStencilInfo;

		return *this;
	}
};
#endif
