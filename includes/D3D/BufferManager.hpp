#ifndef BUFFER_MANAGER_HPP_
#define BUFFER_MANAGER_HPP_
#include <D3DHeaders.hpp>
#include <D3DDescriptorView.hpp>
#include <memory>
#include <vector>
#include <RootSignatureDynamic.hpp>
#include <IModel.hpp>
#include <optional>

class BufferManager {
public:
	struct Args {
		std::optional<std::uint32_t> frameCount;
	};

public:
	BufferManager(const Args& arguments);

	void Update(size_t frameIndex) const noexcept;
	void BindBuffersToGraphics(
		ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex
	) const noexcept;
	void BindBuffersToCompute(
		ID3D12GraphicsCommandList* computeCmdList, size_t frameIndex
	) const noexcept;
	void BindPixelOnlyBuffers(
		ID3D12GraphicsCommandList* graphicsCmdList, size_t frameIndex
	) const noexcept;

	void SetComputeRootSignatureLayout(RSLayoutType rsLayout) noexcept;
	void SetGraphicsRootSignatureLayout(RSLayoutType rsLayout) noexcept;

	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void ReserveBuffers(ID3D12Device* device) noexcept;
	void CreateBuffers(ID3D12Device* device);

private:
	void SetMemoryAddresses() noexcept;
	void UpdateCameraData(size_t bufferIndex) const noexcept;
	void UpdatePerModelData(size_t bufferIndex) const noexcept;
	void UpdateLightData(size_t bufferIndex) const noexcept;
	void UpdatePixelData(size_t bufferIndex) const noexcept;

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

	template<typename T>
	void CopyStruct(
		const T& data, std::uint8_t* offsetInMemory, size_t& currentOffset
	) const noexcept {
		static constexpr size_t stride = sizeof(T);

		memcpy(offsetInMemory + currentOffset, &data, stride);
		currentOffset += stride;
	}

private:
	D3DRootDescriptorView m_cameraBuffer;
	D3DRootDescriptorView m_pixelDataBuffer;
	D3DDescriptorView m_modelBuffers;
	D3DDescriptorView m_materialBuffers;
	D3DDescriptorView m_lightBuffers;

	RSLayoutType m_graphicsRSLayout;
	RSLayoutType m_computeRSLayout;

	std::vector<std::shared_ptr<IModel>> m_opaqueModels;
	std::uint32_t m_frameCount;
	std::vector<size_t> m_lightModelIndices;
};

struct ModelBuffer {
	UVInfo uvInfo;
	DirectX::XMMATRIX modelMatrix;
	std::uint32_t textureIndex;
	DirectX::XMFLOAT3 modelOffset;
	ModelBounds boundingBox;
};

struct MaterialBuffer {
	DirectX::XMFLOAT4 ambient;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	float shininess;
};

struct LightBuffer {
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 ambient;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
};

struct PixelData {
	std::uint32_t lightCount;
};
#endif