#ifndef DEPTH_BUFFER_HPP_
#define DEPTH_BUFFER_HPP_
#include <D3DHeaders.hpp>
#include <cstdint>
#include <numeric>
#include <D3DResources.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DDescriptorHeapManager.hpp>

class DepthBuffer
{
public:
	DepthBuffer(
		ID3D12Device* device, MemoryManager* memoryManager, D3DReusableDescriptorHeap* dsvHeap
	) : m_device{ device }, m_dsvHeap{ dsvHeap },
		m_depthTexture{ device, memoryManager, D3D12_HEAP_TYPE_DEFAULT },
		m_dsvHandleIndex{ std::numeric_limits<UINT>::max() }
	{}
	~DepthBuffer() noexcept;

	void Create(UINT width, UINT height);
	void ClearDSV(const D3DCommandList& commandList) const noexcept;

	[[nodiscard]]
	const Texture& GetDepthTexture() const noexcept { return m_depthTexture; }

private:
	ID3D12Device*              m_device;
	D3DReusableDescriptorHeap* m_dsvHeap;
	Texture                    m_depthTexture;
	UINT                       m_dsvHandleIndex;

public:
	DepthBuffer(const DepthBuffer&) = delete;
	DepthBuffer& operator=(const DepthBuffer&) = delete;

	DepthBuffer(DepthBuffer&& other) noexcept
		: m_device{ other.m_device }, m_dsvHeap{ other.m_dsvHeap },
		m_depthTexture{ std::move(other.m_depthTexture) },
		m_dsvHandleIndex{ other.m_dsvHandleIndex }
	{}
	DepthBuffer& operator=(DepthBuffer&& other) noexcept
	{
		m_device         = other.m_device;
		m_dsvHeap        = other.m_dsvHeap;
		m_depthTexture   = std::move(other.m_depthTexture);
		m_dsvHandleIndex = other.m_dsvHandleIndex;

		return *this;
	}
};
#endif
