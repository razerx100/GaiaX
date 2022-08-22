#ifndef BUFFER_VIEW_HPP_
#define BUFFER_VIEW_HPP_
#include <D3DHeaders.hpp>

template<typename TBufferView>
class BufferView {
public:
	BufferView() = default;
	BufferView(const TBufferView& bufferView)
		: m_bufferView{ bufferView } {}

	void AddBufferView(const TBufferView& bufferView) noexcept {
		m_bufferView = bufferView;
	}

	void OffsetGPUAddress(D3D12_GPU_VIRTUAL_ADDRESS offset) noexcept {
		m_bufferView.BufferLocation += offset;
	}

	const TBufferView* GetAddress() const noexcept {
		return &m_bufferView;
	}

private:
	TBufferView m_bufferView;
};
#endif
