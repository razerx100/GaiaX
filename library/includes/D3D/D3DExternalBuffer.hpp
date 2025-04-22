#ifndef D3D_EXTERNAL_BUFFER_HPP_
#define D3D_EXTERNAL_BUFFER_HPP_
#include <array>
#include <ExternalBuffer.hpp>
#include <D3DResources.hpp>
#include <D3DResourceBarrier.hpp>
#include <D3DRenderingAttachments.hpp>

namespace Gaia
{
class D3DExternalBuffer : public ExternalBuffer
{
public:
	D3DExternalBuffer(
		ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType,
		D3D12_RESOURCE_STATES resourceState,
		D3D12_RESOURCE_FLAGS bufferFlag = D3D12_RESOURCE_FLAG_NONE
	);

	void Create(size_t bufferSize) override;

	void Destroy() noexcept override { m_buffer.Destroy(); }

	[[nodiscard]]
	size_t BufferSize() const noexcept override { return m_buffer.BufferSize(); }

	[[nodiscard]]
	std::uint8_t* CPUHandle() const override { return m_buffer.CPUHandle(); }

	[[nodiscard]]
	const Buffer& GetBuffer() const noexcept { return m_buffer; }

private:
	Buffer                m_buffer;
	D3D12_RESOURCE_STATES m_resourceState;
	D3D12_RESOURCE_FLAGS  m_bufferFlag;

public:
	D3DExternalBuffer(const D3DExternalBuffer&) = delete;
	D3DExternalBuffer& operator=(const D3DExternalBuffer&) = delete;

	D3DExternalBuffer(D3DExternalBuffer&& other) noexcept
		: m_buffer{ std::move(other.m_buffer) }, m_resourceState{ other.m_resourceState },
		m_bufferFlag{ other.m_bufferFlag }
	{}
	D3DExternalBuffer& operator=(D3DExternalBuffer&& other) noexcept
	{
		m_buffer        = std::move(other.m_buffer);
		m_resourceState = other.m_resourceState;
		m_bufferFlag    = other.m_bufferFlag;

		return *this;
	}
};

class D3DExternalTexture : public ExternalTexture
{
public:
	D3DExternalTexture(ID3D12Device* device, MemoryManager* memoryManager);

	void Create(
		std::uint32_t width, std::uint32_t height, ExternalFormat format,
		ExternalTexture2DType type, const ExternalTextureCreationFlags& creationFlags = {}
	) override;

	void SetAttachmentHeap(D3DReusableDescriptorHeap* attachmentHeap) noexcept;

	void SetRenderTargetClearColour(const std::array<float, 4u>& colour) noexcept;

	void AddDSVFlag(D3D12_DSV_FLAGS dsvFlag) noexcept;
	void SetDSVFlag(D3D12_DSV_FLAGS dsvFlag) noexcept;

	void SetDepthStencilClearColour(const D3D12_DEPTH_STENCIL_VALUE& depthStencilColour) noexcept;
	void SetDepthClearColour(float depthColour) noexcept;
	void SetStencilClearColour(UINT8 stencilColour) noexcept;

	void Recreate(ExternalTexture2DType type);

	void Destroy() noexcept override { m_texture.Destroy(); }

	void SetCurrentState(D3D12_RESOURCE_STATES newState) noexcept
	{
		m_currentState = newState;
	}

	[[nodiscard]]
	Extent GetExtent() const noexcept override
	{
		return Extent
		{
			.width  = static_cast<std::uint32_t>(m_texture.GetWidth()),
			.height = m_texture.GetHeight()
		};
	}

	[[nodiscard]]
	bool IsTextureCreated() const noexcept { return m_texture.Get(); }

	[[nodiscard]]
	D3D12_RESOURCE_STATES GetCurrentState() const noexcept { return m_currentState; }

	[[nodiscard]]
	const Texture& GetTexture() const noexcept { return m_texture; }

	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetAttachmentHandle() const noexcept
	{
		return m_renderingAttachment.GetCPUHandle();
	}

	[[nodiscard]]
	UINT GetDSVFlags() const noexcept { return m_renderingAttachment.GetDSVFlags(); }

private:
	Texture               m_texture;
	D3D12_RESOURCE_STATES m_currentState;
	D3D12_CLEAR_VALUE     m_clearValue;
	RenderingAttachment   m_renderingAttachment;

public:
	D3DExternalTexture(const D3DExternalTexture&) = delete;
	D3DExternalTexture& operator=(const D3DExternalTexture&) = delete;

	D3DExternalTexture(D3DExternalTexture&& other) noexcept
		: m_texture{ std::move(other.m_texture) },
		m_currentState{ other.m_currentState },
		m_clearValue{ other.m_clearValue },
		m_renderingAttachment{ std::move(other.m_renderingAttachment) }
	{}
	D3DExternalTexture& operator=(D3DExternalTexture&& other) noexcept
	{
		m_texture             = std::move(other.m_texture);
		m_currentState        = other.m_currentState;
		m_clearValue          = other.m_clearValue;
		m_renderingAttachment = std::move(other.m_renderingAttachment);

		return *this;
	}
};
}
#endif
