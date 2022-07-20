#ifndef DESCRIPTOR_TABLE_MANAGER_HPP_
#define DESCRIPTOR_TABLE_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <vector>
#include <GaiaDataTypes.hpp>

using SharedDescriptorHandles = std::pair<SharedCPUHandle, SharedGPUHandle>;

class DescriptorTableManager {
public:
	DescriptorTableManager();

	void CreateDescriptorTable(ID3D12Device* device);
	void CopyUploadHeap(ID3D12Device* device);
	void ReleaseUploadHeap() noexcept;

	[[nodiscard]]
	SharedCPUHandle ReserveDescriptorsTexture(size_t descriptorCount = 1u) noexcept;
	[[nodiscard]]
	SharedDescriptorHandles ReserveDescriptors(size_t descriptorCount = 1u) noexcept;

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

private:
	size_t m_descriptorCount;
	D3D12_GPU_DESCRIPTOR_HANDLE m_textureRangeStart;
	ComPtr<ID3D12DescriptorHeap> m_pDescHeap;
	ComPtr<ID3D12DescriptorHeap> m_uploadDescHeap;
	std::vector<SharedCPUHandle> m_sharedTextureCPUHandle;
	std::vector<size_t> m_textureDescriptorCounts;
	std::vector<SharedCPUHandle> m_genericCPUHandles;
	std::vector<SharedGPUHandle> m_genericGPUHandles;
	std::vector<size_t> m_genericDescriptorCounts;
};
#endif
