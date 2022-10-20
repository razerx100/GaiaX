#ifndef ROOT_SIGNATURE_DYNAMIC_HPP_
#define ROOT_SIGNATURE_DYNAMIC_HPP_
#include <RootSignatureBase.hpp>
#include <vector>
#include <memory>

enum class RootSigElement {
	Camera,
	ModelData,
	Textures,
	TextureIndex,
	IndirectArgsSRV,
	ElementCount
};

class RootSignatureDynamic final : public RootSignatureBase {
public:
	RootSignatureDynamic() noexcept;

	void AddConstants(
		std::uint32_t numOfValues, D3D12_SHADER_VISIBILITY visibility,
		RootSigElement elementType, std::uint32_t registerNumber,
		std::uint32_t registerSpace = 0u
	) noexcept;

	void AddDescriptorTable(
		D3D12_DESCRIPTOR_RANGE_TYPE descriptorType, std::uint32_t descriptorsAmount,
		D3D12_SHADER_VISIBILITY visibility, RootSigElement elementType,
		bool bindless, std::uint32_t registerNumber, std::uint32_t registerSpace = 0u
	) noexcept;

	void AddConstantBufferView(
		D3D12_SHADER_VISIBILITY visibility, RootSigElement elementType,
		std::uint32_t registerNumber,
		std::uint32_t registerSpace = 0u
	) noexcept;

	void AddUnorderedAccessView(
		D3D12_SHADER_VISIBILITY visibility, RootSigElement elementType,
		std::uint32_t registerNumber, std::uint32_t registerSpace = 0u
	) noexcept;

	void AddShaderResourceView(
		D3D12_SHADER_VISIBILITY visibility, RootSigElement elementType,
		std::uint32_t registerNumber, std::uint32_t registerSpace = 0u
	) noexcept;

	void CompileSignature(bool staticSampler = true);

	[[nodiscard]]
	std::vector<UINT> GetElementLayout() const noexcept;

private:
	void AddElementType(RootSigElement elementType) noexcept;

private:
	std::vector<D3D12_ROOT_PARAMETER1> m_rootParameters;
	std::vector<std::unique_ptr<D3D12_DESCRIPTOR_RANGE1>> m_rangePreserver;
	std::vector<UINT> m_elementLayout;
};
#endif
