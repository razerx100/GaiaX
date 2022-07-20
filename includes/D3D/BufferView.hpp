#ifndef BUFFER_VIEW_HPP_
#define BUFFER_VIEW_HPP_
#include <GaiaDataTypes.hpp>

template<typename TBufferView>
class BufferView {
public:
	BufferView() = default;
	BufferView(
		TBufferView&& bufferView, D3DGPUSharedAddress sharedAddress
	) : m_bufferView(std::move(bufferView)), m_gpuSharedAddress(sharedAddress) {}

	void SetGPUAddress() noexcept {
		m_bufferView.BufferLocation = *m_gpuSharedAddress;
	}
	void AddBufferView(TBufferView&& bufferView) noexcept {
		m_bufferView = std::move(bufferView);
	}
	void AddSharedAddress(D3DGPUSharedAddress sharedAddress) noexcept {
		m_gpuSharedAddress = sharedAddress;
	}

	const TBufferView* GetAddress() const noexcept {
		return &m_bufferView;
	}

private:
	TBufferView m_bufferView;
	D3DGPUSharedAddress m_gpuSharedAddress;
};
#endif
