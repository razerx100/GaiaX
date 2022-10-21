#ifndef PER_FRAME_BUFFERS_HPP_
#define PER_FRAME_BUFFERS_HPP_
#include <D3DHeaders.hpp>
#include <BufferView.hpp>
#include <D3DDescriptorView.hpp>
#include <memory>
#include <vector>
#include <RootSignatureDynamic.hpp>

class PerFrameBuffers {
public:
	PerFrameBuffers(std::uint32_t frameCount);

	void UpdateData(size_t frameIndex) const noexcept;
	void BindPerFrameBuffersToGraphics(
		ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex,
		const std::vector<UINT>& rsLayout
	) const noexcept;
	void BindPerFrameBuffersToCompute(
		ID3D12GraphicsCommandList* computeCmdList, size_t frameIndex,
		const std::vector<UINT>& rsLayout
	) const noexcept;
	void BindVertexBuffer(ID3D12GraphicsCommandList* graphicsCmdList) const noexcept;
	void SetMemoryAddresses() noexcept;

	void AddModelInputs(
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	);

private:
	void InitBuffers(std::uint32_t frameCount);

	template<void (__stdcall ID3D12GraphicsCommandList::*RCBV)(UINT, D3D12_GPU_VIRTUAL_ADDRESS)>
	void BindPerFrameBuffers(
		ID3D12GraphicsCommandList* cmdList, size_t frameIndex, const std::vector<UINT>& rsLayout
	) const noexcept {
		static constexpr size_t cameraTypeIndex = static_cast<size_t>(RootSigElement::Camera);

		(cmdList->*RCBV)(
			rsLayout[cameraTypeIndex], m_cameraBuffer.GetGPUAddressStart(frameIndex)
			);
	}

private:
	D3DRootDescriptorView m_cameraBuffer;
	BufferView<D3D12_VERTEX_BUFFER_VIEW> m_gVertexBufferView;
	BufferView<D3D12_INDEX_BUFFER_VIEW> m_gIndexBufferView;
};
#endif