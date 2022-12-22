#ifndef BUFFER_MANAGER_HPP_
#define BUFFER_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <D3DDescriptorView.hpp>
#include <memory>
#include <vector>
#include <RootSignatureDynamic.hpp>
#include <IModel.hpp>

struct ModelBuffer {
	UVInfo uvInfo;
	DirectX::XMMATRIX modelMatrix;
	std::uint32_t textureIndex;
	DirectX::XMFLOAT3 modelOffset;
	ModelBounds boundingBox;
};

class BufferManager {
public:
	BufferManager(std::uint32_t frameCount);

	void Update(size_t frameIndex) const noexcept;
	void BindBuffersToGraphics(
		ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex
	) const noexcept;
	void BindBuffersToCompute(
		ID3D12GraphicsCommandList* computeCmdList, size_t frameIndex
	) const noexcept;

	void SetComputeRootSignatureLayout(RSLayoutType rsLayout) noexcept;
	void SetGraphicsRootSignatureLayout(RSLayoutType rsLayout) noexcept;

	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void ReserveBuffers(ID3D12Device* device) noexcept;
	void CreateBuffers(ID3D12Device* device);

private:
	void SetMemoryAddresses() noexcept;
	void UpdateModelData(size_t frameIndex) const noexcept;

	template<void (__stdcall ID3D12GraphicsCommandList::*RCBV)(UINT, D3D12_GPU_VIRTUAL_ADDRESS),
	void (__stdcall ID3D12GraphicsCommandList::*RDT)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)>
	void BindBuffers(
		ID3D12GraphicsCommandList* cmdList, size_t frameIndex, const RSLayoutType& rsLayout
	) const noexcept {
		static constexpr auto cameraTypeIndex = static_cast<size_t>(RootSigElement::Camera);
		(cmdList->*RCBV)(
			rsLayout[cameraTypeIndex], m_cameraBuffer.GetGPUAddressStart(frameIndex)
			);

		static constexpr auto modelBufferTypeIndex =
			static_cast<size_t>(RootSigElement::ModelData);
		(cmdList->*RDT)(
			rsLayout[modelBufferTypeIndex], m_modelBuffers.GetGPUDescriptorHandle(frameIndex)
		);
	}

private:
	D3DRootDescriptorView m_cameraBuffer;

	RSLayoutType m_graphicsRSLayout;
	RSLayoutType m_computeRSLayout;

	std::vector<std::shared_ptr<IModel>> m_opaqueModels;
	D3DSingleDescriptorView m_modelBuffers;
	std::uint32_t m_frameCount;
};
#endif