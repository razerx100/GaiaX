#ifndef D3D_EXTERNAL_BUFFER_HPP_
#define D3D_EXTERNAL_BUFFER_HPP_
#include <ExternalBuffer.hpp>
#include <D3DResources.hpp>
#include <D3DResourceBarrier.hpp>

class D3DExternalBuffer : public ExternalBuffer
{
public:
	D3DExternalBuffer(
		ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType,
		D3D12_RESOURCE_STATES resourceState, D3D12_RESOURCE_FLAGS bufferFlag = D3D12_RESOURCE_FLAG_NONE
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
		std::uint32_t width, std::uint32_t height, ExternalFormat format, ExternalTexture2DType type,
		bool copySrc, bool copyDst, const ExternalClearColour& clearColour = {}
	) override;

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
	// This actually won't change the state. It would just replace the older state with the new one.
	// And keep track of it.
	ResourceBarrierBuilder TransitionState(D3D12_RESOURCE_STATES newState) noexcept;

	[[nodiscard]]
	D3D12_RESOURCE_STATES GetCurrentState() const noexcept { return m_currentState; }

	[[nodiscard]]
	const Texture& GetTexture() const noexcept { return m_texture; }

private:
	Texture               m_texture;
	D3D12_RESOURCE_STATES m_currentState;

public:
	D3DExternalTexture(const D3DExternalTexture&) = delete;
	D3DExternalTexture& operator=(const D3DExternalTexture&) = delete;

	D3DExternalTexture(D3DExternalTexture&& other) noexcept
		: m_texture{ std::move(other.m_texture) },
		m_currentState{ other.m_currentState }
	{}
	D3DExternalTexture& operator=(D3DExternalTexture&& other) noexcept
	{
		m_texture      = std::move(other.m_texture);
		m_currentState = other.m_currentState;

		return *this;
	}
};
#endif
