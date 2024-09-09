#ifndef DEPTH_BUFFER_HPP_
#define DEPTH_BUFFER_HPP_
#include <D3DHeaders.hpp>
#include <cstdint>
#include <D3DResources.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DDescriptorHeapManager.hpp>

class DepthBuffer
{
public:
	DepthBuffer(ID3D12Device* device, MemoryManager* memoryManager)
		: m_device{ device }, m_depthTexture{ device, memoryManager, D3D12_HEAP_TYPE_DEFAULT },
		m_dsvHandleIndex{ 0u }
	{}

	void Create(D3DReusableDescriptorHeap& descriptorHeap, UINT width, UINT height);
	void ClearDSV(
		const D3DCommandList& commandList, const D3DReusableDescriptorHeap& descriptorHeap
	) const noexcept;

	[[nodiscard]]
	const Texture& GetDepthTexture() const noexcept { return m_depthTexture; }
	[[nodiscard]]
	UINT GetDescriptorIndex() const noexcept { return m_dsvHandleIndex; }

private:
	ID3D12Device* m_device;
	Texture       m_depthTexture;
	UINT          m_dsvHandleIndex;

public:
	DepthBuffer(const DepthBuffer&) = delete;
	DepthBuffer& operator=(const DepthBuffer&) = delete;

	DepthBuffer(DepthBuffer&& other) noexcept
		: m_device{ other.m_device }, m_depthTexture{ std::move(other.m_depthTexture) },
		m_dsvHandleIndex{ other.m_dsvHandleIndex }
	{}
	DepthBuffer& operator=(DepthBuffer&& other) noexcept
	{
		m_device         = other.m_device;
		m_depthTexture   = std::move(other.m_depthTexture);
		m_dsvHandleIndex = other.m_dsvHandleIndex;

		return *this;
	}
};
#endif
