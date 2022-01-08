#ifndef __I_COPY_QUEUE_MANAGER_HPP__
#define __I_COPY_QUEUE_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <cstdint>

class ICopyQueueManager {
public:
	virtual ~ICopyQueueManager() = default;

	virtual void InitSyncObjects(ID3D12Device* device) = 0;
	virtual void WaitForGPU() = 0;
	virtual void ExecuteCommandLists(
		ID3D12GraphicsCommandList* commandList
	) const noexcept = 0;

	virtual ID3D12CommandQueue* GetQueueRef() const noexcept = 0;
};

ICopyQueueManager* CreateCopyQueueInstance(
	ID3D12Device* device
);

#endif