#ifndef DESCRIPTOR_TABLE_MANAGER_HPP_
#define DESCRIPTOR_TABLE_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <SharedAddress.hpp>
#include <vector>
#include <memory>

using SharedIndex = std::shared_ptr<SharedAddress>;
using SharedCPUHandle = std::shared_ptr<_SharedAddress<SIZE_T>>;
using SharedGPUHandle = std::shared_ptr<_SharedAddress<UINT64>>;
using ResourceAddress = std::pair<SharedIndex, SharedCPUHandle>;
using SharedDescriptorHandles = std::pair<SharedCPUHandle, SharedGPUHandle>;

class DescriptorTableManager {
public:
	DescriptorTableManager();

	void CreateDescriptorTable(ID3D12Device* device);
	void CopyUploadHeap(ID3D12Device* device);
	void ReleaseUploadHeap() noexcept;

	[[nodiscard]]
	ResourceAddress ReserveDescriptorTexture() noexcept;
	[[nodiscard]]
	SharedDescriptorHandles ReserveDescriptor() noexcept;

	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureRangeStart() const noexcept;
	[[nodiscard]]
	size_t GetTextureDescriptorCount() const noexcept;
	[[nodiscard]]
	ID3D12DescriptorHeap* GetDescHeapRef() const noexcept;

private:
	ComPtr<ID3D12DescriptorHeap> CreateDescHeap(
		ID3D12Device* device, size_t descriptorCount, bool shaderVisible = true
	) const;

	void SetSharedAddresses(
		std::vector<SharedCPUHandle>& sharedHandles,
		std::vector<SharedIndex>& sharedIndices,
		SIZE_T& cpuHandle, SIZE_T descIncSize,
		size_t indicesStart
	) const noexcept;

private:
	size_t m_descriptorCount;
	D3D12_GPU_DESCRIPTOR_HANDLE m_textureRangeStart;
	ComPtr<ID3D12DescriptorHeap> m_pDescHeap;
	ComPtr<ID3D12DescriptorHeap> m_uploadDescHeap;
	std::vector<SharedCPUHandle> m_sharedTextureCPUHandle;
	std::vector<SharedIndex> m_sharedTextureIndices;
	std::vector<SharedCPUHandle> m_genericCPUHandles;
	std::vector<SharedGPUHandle> m_genericGPUHandles;
};
#endif
