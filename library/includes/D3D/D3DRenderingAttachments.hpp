#ifndef D3D_RENDERING_ATTACHMENTS_HPP_
#define D3D_RENDERING_ATTACHMENTS_HPP_
#include <D3DHeaders.hpp>
#include <numeric>
#include <utility>
#include <D3DDescriptorHeapManager.hpp>

class RenderingAttachment
{
public:
	RenderingAttachment()
		: m_attachmentHeap{ nullptr }, m_descriptorIndex{ std::numeric_limits<UINT>::max() },
		m_dsvFlags{ D3D12_DSV_FLAG_NONE }
	{}
	~RenderingAttachment() noexcept;

	void SetAttachmentHeap(D3DReusableDescriptorHeap* attachmentHeap) noexcept
	{
		m_attachmentHeap = attachmentHeap;
	}

	void AddDSVFlag(D3D12_DSV_FLAGS dsvFlag) noexcept { m_dsvFlags |= dsvFlag; }

	void CreateRTV(ID3D12Resource* renderTarget, DXGI_FORMAT rtvFormat);

	void CreateDSV(ID3D12Resource* depthStencilTarget, DXGI_FORMAT dsvFormat);

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept
	{
		return m_attachmentHeap->GetCPUHandle(m_descriptorIndex);
	}

private:
	D3DReusableDescriptorHeap* m_attachmentHeap;
	UINT                       m_descriptorIndex;
	UINT                       m_dsvFlags;

public:
	RenderingAttachment(const RenderingAttachment&) = delete;
	RenderingAttachment& operator=(const RenderingAttachment&) = delete;

	RenderingAttachment(RenderingAttachment&& other) noexcept
		: m_attachmentHeap{ std::exchange(other.m_attachmentHeap, nullptr) },
		m_descriptorIndex{ other.m_descriptorIndex }, m_dsvFlags{ other.m_dsvFlags }
	{}

	RenderingAttachment& operator=(RenderingAttachment&& other) noexcept
	{
		m_attachmentHeap  = std::exchange(other.m_attachmentHeap, nullptr);
		m_descriptorIndex = other.m_descriptorIndex;
		m_dsvFlags        = other.m_dsvFlags;

		return *this;
	}
};
#endif
