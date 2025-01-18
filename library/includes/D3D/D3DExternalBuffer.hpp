#ifndef D3D_EXTERNAL_BUFFER_HPP_
#define D3D_EXTERNAL_BUFFER_HPP_
#include <ExternalBuffer.hpp>
#include <D3DResources.hpp>

class D3DExternalBuffer : public ExternalBuffer
{
public:
	D3DExternalBuffer(
		ID3D12Device* device, MemoryManager* memoryManager, D3D12_HEAP_TYPE memoryType,
		D3D12_RESOURCE_STATES resourceState, D3D12_RESOURCE_FLAGS bufferFlag = D3D12_RESOURCE_FLAG_NONE
	);

	void Create(size_t bufferSize) override;

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
#endif
