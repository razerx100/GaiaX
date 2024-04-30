#ifndef DESCRIPTOR_TABLE_MANAGER_HPP_
#define DESCRIPTOR_TABLE_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <vector>

class DescriptorTableManager {
public:
	DescriptorTableManager();

	void CreateDescriptorTable(ID3D12Device* device);
	void CopyUploadHeap(ID3D12Device* device);
	void ReleaseUploadHeap() noexcept;

	[[nodiscard]]
	size_t ReserveDescriptorsTextureAndGetRelativeOffset(size_t descriptorCount = 1u) noexcept;
	[[nodiscard]]
	size_t ReserveDescriptorsAndGetOffset(size_t descriptorCount = 1u) noexcept;

	[[nodiscard]]
	size_t GetTextureRangeStart() const noexcept;
	[[nodiscard]]
	size_t GetTextureDescriptorCount() const noexcept;
	[[nodiscard]]
	ID3D12DescriptorHeap* GetDescHeapRef() const noexcept;
	[[nodiscard]]
	D3D12_CPU_DESCRIPTOR_HANDLE GetUploadDescriptorStart() const noexcept;
	[[nodiscard]]
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorStart() const noexcept;

private:
	ComPtr<ID3D12DescriptorHeap> CreateDescHeap(
		ID3D12Device* device, size_t descriptorCount, bool shaderVisible = true
	) const;

private:
	size_t m_genericDescriptorCount;
	size_t m_textureDescriptorCount;
	ComPtr<ID3D12DescriptorHeap> m_pDescHeap;
	ComPtr<ID3D12DescriptorHeap> m_uploadDescHeap;
	std::vector<size_t> m_textureDescriptorSet;
	std::vector<size_t> m_genericDescriptorSet;
};
#endif
