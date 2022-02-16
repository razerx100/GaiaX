#ifndef __DESCRIPTOR_TABLE_MANAGER_HPP__
#define __DESCRIPTOR_TABLE_MANAGER_HPP__
#include <IDescriptorTableManager.hpp>
#include <vector>

class DescriptorTableManager : public IDescriptorTableManager {
public:
	DescriptorTableManager();

	void CreateDescriptorTable(ID3D12Device* device) override;
	void CopyUploadHeap(ID3D12Device* device) override;
	void ReleaseUploadHeap() noexcept override;

	ResourceAddress GetColorIndex() noexcept override;

	D3D12_GPU_DESCRIPTOR_HANDLE GetColorRangeStart() const noexcept override;
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureRangeStart() const noexcept override;

	size_t GetColorDescriptorCount() const noexcept override;
	size_t GetTextureDescriptorCount() const noexcept override;

	ID3D12DescriptorHeap* GetDescHeapRef() const noexcept override;

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
	D3D12_GPU_DESCRIPTOR_HANDLE m_colorRangeStart;
	D3D12_GPU_DESCRIPTOR_HANDLE m_textureRangeStart;
	ComPtr<ID3D12DescriptorHeap> m_pDescHeap;
	ComPtr<ID3D12DescriptorHeap> m_uploadDescHeap;
	std::vector<SharedCPUHandle> m_sharedColorCPUHandle;
	std::vector<SharedIndex> m_sharedColorIndices;
};
#endif
