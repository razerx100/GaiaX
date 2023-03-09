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
		std::optional<bool> meshletModelData;
	};

public:
	BufferManager(const Args& arguments);

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

	template<bool meshletModel>
	void Update(size_t frameIndex) const noexcept {
		const DirectX::XMMATRIX viewMatrix = GetViewMatrix();

		UpdateCameraData(frameIndex);
		UpdatePerModelData<meshletModel>(frameIndex, viewMatrix);
		UpdateLightData(frameIndex, viewMatrix);
		UpdatePixelData(frameIndex);
	}

private:
	struct ModelBuffer {
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMMATRIX viewNormalMatrix;
		DirectX::XMFLOAT3 modelOffset;
		ModelBounds boundingBox;
	};

	struct ModelBufferMesh {
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMMATRIX viewNormalMatrix;
		DirectX::XMFLOAT3 modelOffset;
	};

	struct MaterialBuffer {
		DirectX::XMFLOAT4 ambient;
		DirectX::XMFLOAT4 diffuse;
		DirectX::XMFLOAT4 specular;
		UVInfo diffuseTexUVInfo;
		UVInfo specularTexUVInfo;
		std::uint32_t diffuseTexIndex;
		std::uint32_t specularTexIndex;
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

private:
	[[nodiscard]]
	DirectX::XMMATRIX GetViewMatrix() const noexcept;

	void SetMemoryAddresses() noexcept;
	void UpdateCameraData(size_t bufferIndex) const noexcept;
	void UpdateLightData(
		size_t bufferIndex, const DirectX::XMMATRIX& viewMatrix
	) const noexcept;
	void UpdatePixelData(size_t bufferIndex) const noexcept;

	template<bool meshletModel>
	void UpdatePerModelData(
		size_t bufferIndex, const DirectX::XMMATRIX& viewMatrix
	) const noexcept {
		size_t modelOffset = 0u;
		std::uint8_t* modelBufferOffset = m_modelBuffers.GetCPUWPointer(bufferIndex);

		size_t materialOffset = 0u;
		std::uint8_t* materialBufferOffset = m_materialBuffers.GetCPUWPointer(bufferIndex);

		for (auto& model : m_opaqueModels) {
			const DirectX::XMMATRIX modelMatrix = model->GetModelMatrix();

			if constexpr (meshletModel) {
				ModelBufferMesh modelBuffer{
					.modelMatrix = modelMatrix,
					.viewNormalMatrix = DirectX::XMMatrixTranspose(
						DirectX::XMMatrixInverse(nullptr, modelMatrix * viewMatrix)
					),
					.modelOffset = model->GetModelOffset()
				};
				CopyStruct(modelBuffer, modelBufferOffset, modelOffset);
			}
			else {
				ModelBuffer modelBuffer{
					.modelMatrix = modelMatrix,
					.viewNormalMatrix = DirectX::XMMatrixTranspose(
						DirectX::XMMatrixInverse(nullptr, modelMatrix * viewMatrix)
					),
					.modelOffset = model->GetModelOffset(),
					.boundingBox = model->GetBoundingBox()
				};
				CopyStruct(modelBuffer, modelBufferOffset, modelOffset);
			}

			const auto& modelMaterial = model->GetMaterial();

			MaterialBuffer material{
				.ambient = modelMaterial.ambient,
				.diffuse = modelMaterial.diffuse,
				.specular = modelMaterial.specular,
				.diffuseTexUVInfo = model->GetDiffuseTexUVInfo(),
				.specularTexUVInfo = model->GetSpecularTexUVInfo(),
				.diffuseTexIndex = model->GetDiffuseTexIndex(),
				.specularTexIndex = model->GetSpecularTexIndex(),
				.shininess = modelMaterial.shininess
			};
			CopyStruct(material, materialBufferOffset, materialOffset);
		}
	}

	template<void(__stdcall ID3D12GraphicsCommandList::* RCBV)(UINT, D3D12_GPU_VIRTUAL_ADDRESS),
		void(__stdcall ID3D12GraphicsCommandList::* RDT)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)>
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
	bool m_meshletModelData;
};
#endif