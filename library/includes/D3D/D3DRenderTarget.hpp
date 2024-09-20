#ifndef D3D_RENDER_TARGET_HPP_
#define D3D_RENDER_TARGET_HPP_
#include <D3DHeaders.hpp>
#include <array>
#include <numeric>
#include <D3DResources.hpp>
#include <D3DCommandQueue.hpp>
#include <D3DDescriptorHeapManager.hpp>

class RenderTarget
{
public:
	RenderTarget(D3DReusableDescriptorHeap* rtvHeap)
		: m_rtvHeap{ rtvHeap }, m_renderTarget{}, m_descriptorIndex{ std::numeric_limits<UINT>::max() }
	{}
	~RenderTarget() noexcept;

	void Create(ComPtr<ID3D12Resource>&& renderTarget);

	void Set(
		const D3DCommandList& commandList, const std::array<float, 4u>& clearColour,
		D3D12_CPU_DESCRIPTOR_HANDLE const* dsvHandle
	) const;

	void ToRenderState(const D3DCommandList& commandList) const noexcept;
	void ToPresentState(const D3DCommandList& commandList) const noexcept;

	void Reset() { m_renderTarget.Reset(); }

	[[nodiscard]]
	ID3D12Resource* Get() const noexcept { return m_renderTarget.Get(); }

private:
	D3DReusableDescriptorHeap* m_rtvHeap;
	ComPtr<ID3D12Resource>     m_renderTarget;
	UINT                       m_descriptorIndex;

public:
	RenderTarget(const RenderTarget&) = delete;
	RenderTarget& operator=(const RenderTarget&) = delete;

	RenderTarget(RenderTarget&& other) noexcept
		: m_rtvHeap{ other.m_rtvHeap },
		m_renderTarget{ std::move(other.m_renderTarget) },
		m_descriptorIndex{ other.m_descriptorIndex }
	{}

	RenderTarget& operator=(RenderTarget&& other) noexcept
	{
		m_rtvHeap         = other.m_rtvHeap;
		m_renderTarget    = std::move(other.m_renderTarget);
		m_descriptorIndex = other.m_descriptorIndex;

		return *this;
	}
};
#endif
