#ifndef __I_GRAPHICS_QUEUE_MANAGER_HPP__
#define __I_GRAPHICS_QUEUE_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <cstdint>

class IGraphicsQueueManager {
public:
	virtual ~IGraphicsQueueManager() = default;

	virtual void InitSyncObjects(ID3D12Device5* device, std::uint32_t backBufferIndex) = 0;
	virtual void WaitForGPU(std::uint32_t backBufferIndex) = 0;
	virtual void ExecuteCommandLists() const noexcept = 0;
	virtual void RecordCommandList() = 0;
	virtual void CloseCommandList() const = 0;
	virtual void MoveToNextFrame(std::uint32_t backBufferIndex) = 0;
	virtual void ResetFenceValuesWith(std::uint32_t valueIndex) = 0;

	virtual ID3D12CommandQueue* GetQueueRef() const noexcept = 0;
	virtual ID3D12GraphicsCommandList* GetCommandListRef() const noexcept = 0;
};

IGraphicsQueueManager* GetGraphicsQueueInstance() noexcept;
void InitGraphicsQueueInstance(
	ID3D12Device5* device,
	std::uint8_t bufferCount
);
void CleanUpGraphicsQueueInstance() noexcept;
#endif
