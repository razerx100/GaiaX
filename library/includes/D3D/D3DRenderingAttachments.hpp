#ifndef D3D_RENDERING_ATTACHMENTS_HPP_
#define D3D_RENDERING_ATTACHMENTS_HPP_
#include <D3DHeaders.hpp>
#include <numeric>
#include <utility>
#include <D3DDescriptorHeapManager.hpp>

class RenderingAttachment
{
public:
	RenderingAttachment(D3DReusableDescriptorHeap* attachmentHeap)
		: m_attachmentHeap{ attachmentHeap }, m_descriptorIndex{ std::numeric_limits<UINT>::max() },
		m_clearAtStart{ false }
	{}
	~RenderingAttachment() noexcept;

	void SetClearAtStart(bool value) noexcept { m_clearAtStart = value; }

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept
	{
		return m_attachmentHeap->GetCPUHandle(m_descriptorIndex);
	}

	[[nodiscard]]
	bool ShouldClearAtStart() const noexcept { return m_clearAtStart; }

protected:
	D3DReusableDescriptorHeap* m_attachmentHeap;
	UINT                       m_descriptorIndex;
	bool                       m_clearAtStart;

public:
	RenderingAttachment(const RenderingAttachment&) = delete;
	RenderingAttachment& operator=(const RenderingAttachment&) = delete;

	RenderingAttachment(RenderingAttachment&& other) noexcept
		: m_attachmentHeap{ std::exchange(other.m_attachmentHeap, nullptr) },
		m_descriptorIndex{ other.m_descriptorIndex },
		m_clearAtStart{ other.m_clearAtStart }
	{}

	RenderingAttachment& operator=(RenderingAttachment&& other) noexcept
	{
		m_attachmentHeap  = std::exchange(other.m_attachmentHeap, nullptr);
		m_descriptorIndex = other.m_descriptorIndex;
		m_clearAtStart    = other.m_clearAtStart;

		return *this;
	}
};

class RenderTarget : public RenderingAttachment
{
public:
	RenderTarget(D3DReusableDescriptorHeap* rtvHeap) : RenderingAttachment{ rtvHeap } {}

	void Create(ID3D12Resource* renderTarget, DXGI_FORMAT rtvFormat);

public:
	RenderTarget(const RenderTarget&) = delete;
	RenderTarget& operator=(const RenderTarget&) = delete;

	RenderTarget(RenderTarget&& other) noexcept : RenderingAttachment{ std::move(other) } {}

	RenderTarget& operator=(RenderTarget&& other) noexcept
	{
		RenderingAttachment::operator=(std::move(other));

		return *this;
	}
};

class DepthStencilTarget : public RenderingAttachment
{
public:
	DepthStencilTarget(D3DReusableDescriptorHeap* dsvHeap) : RenderingAttachment{ dsvHeap } {}

	void Create(
		ID3D12Resource* depthStencilTarget, DXGI_FORMAT dsvFormat, D3D12_DSV_FLAGS dsvFlag
	);

public:
	DepthStencilTarget(const DepthStencilTarget&) = delete;
	DepthStencilTarget& operator=(const DepthStencilTarget&) = delete;

	DepthStencilTarget(DepthStencilTarget&& other) noexcept : RenderingAttachment{ std::move(other) } {}

	DepthStencilTarget& operator=(DepthStencilTarget&& other) noexcept
	{
		RenderingAttachment::operator=(std::move(other));

		return *this;
	}
};
#endif
