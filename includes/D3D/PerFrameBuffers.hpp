#ifndef PER_FRAME_BUFFERS_HPP_
#define PER_FRAME_BUFFERS_HPP_
#include <D3DHeaders.hpp>
#include <CPUAccessibleStorage.hpp>

class PerFrameBuffers {
public:
	PerFrameBuffers();

	void BindPerFrameBuffers(ID3D12GraphicsCommandList* graphicsCmdList) const noexcept;
	void SetMemoryAddresses() noexcept;

private:
	class PerFrameEntity {
	public:
		void Init(size_t bufferSize) noexcept;
		void SetMemoryAddresses() noexcept;

		[[nodiscard]]
		std::uint8_t* GetCpuHandle() const noexcept;
		[[nodiscard]]
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuHandle() const noexcept;

	private:
		SharedPair m_sharedMemoryHandles;
		std::uint8_t* m_pCpuHandle;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuHandle;
	};

private:
	void InitBuffers();

private:
	PerFrameEntity m_cameraEntity;
};
#endif