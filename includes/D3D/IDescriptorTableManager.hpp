#ifndef __I_DESCRIPTOR_TABLE_MANAGER_HPP__
#define __I_DESCRIPTOR_TABLE_MANAGER_HPP__
#include <D3DHeaders.hpp>
#include <SharedAddress.hpp>
#include <memory>

using SharedIndex = std::shared_ptr<SharedAddress>;
using SharedCPUHandle = std::shared_ptr<_SharedAddress<SIZE_T>>;
using ResourceAddress = std::pair<SharedIndex, SharedCPUHandle>;

class IDescriptorTableManager {
public:
	virtual ~IDescriptorTableManager() = default;

	virtual void CreateDescriptorTable(ID3D12Device* device) = 0;
	virtual void CopyUploadHeap(ID3D12Device* device) = 0;
	virtual void ReleaseUploadHeap() noexcept = 0;

	virtual ResourceAddress GetTextureIndex() noexcept = 0;
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetTextureRangeStart() const noexcept = 0;
	virtual size_t GetTextureDescriptorCount() const noexcept = 0;
	virtual ID3D12DescriptorHeap* GetDescHeapRef() const noexcept = 0;
};

IDescriptorTableManager* CreateDescriptorTableManagerInstance();
#endif
